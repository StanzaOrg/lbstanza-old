#ifdef PLATFORM_WINDOWS
  #include<Windows.h>
#else
  #include<sys/wait.h>
#endif
#include<stdint.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/time.h>
#include<errno.h>
#include<fcntl.h>
#include<signal.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>

//     Stanza Defined Entities
//     =======================
int64_t stanza_entry (void);
extern int64_t stanza_stack_size;
extern int64_t stanza_stack_items_offset;
extern char* stanza_stack_pointer;

//     Command line arguments
//     ======================
int input_argc;
char** input_argv;

//     Main Driver
//     ===========
int main (int argc, char* argv[]) {  
  input_argc = argc;
  input_argv = argv;
  char* stack_mem = (char*)malloc(stanza_stack_size);
  stanza_stack_pointer = stack_mem + stanza_stack_items_offset;
  stanza_entry();
  return 0;
}

//     Macro Readers
//     =============
FILE* get_stdout () {return stdout;}
FILE* get_stderr () {return stderr;}
FILE* get_stdin () {return stdin;}
int get_eof () {return EOF;}
int get_errno () {return errno;}

//     Time of Day
//     ===========
int64_t current_time_us (void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (int64_t)tv.tv_sec * 1000 * 1000 + (int64_t)tv.tv_usec;
}

int64_t current_time_ms (void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000;
}

//     Random Access Files
//     ===================
int64_t get_file_size (FILE* f) {
  int64_t cur_pos = ftell(f);
  fseek(f, 0, SEEK_END);
  int64_t size = ftell(f);
  fseek(f, cur_pos, SEEK_SET);
  return size;
}

int file_seek (FILE* f, int64_t pos) {
  return fseek(f, pos, SEEK_SET);
}

int file_skip (FILE* f, int64_t num) {
  return fseek(f, num, SEEK_CUR);
}

int file_set_length (FILE* f, int64_t size) {
  return ftruncate(fileno(f), size);
}

int64_t file_read_block (FILE* f, char* data, int64_t len) {
  return fread(data, 1, len, f);
}

int64_t file_write_block (FILE* f, char* data, int64_t len) {
  return fwrite(data, 1, len, f);
}


//     Path Resolution
//     ===============
#ifdef PLATFORM_WINDOWS
  int file_exists (char* filename) {
    int attrib = GetFileAttributes(filename);
    return attrib != INVALID_FILE_ATTRIBUTES;
  }

  char* resolve_path (char* filename){
    if(file_exists(filename)){
      char* fileext;
      char* path = (char*)malloc(2048);
      int ret = GetFullPathName(filename, 2048, path, &fileext);
      if(ret == 0){
        free(path);
        return 0;
      }else{
        return path;
      }             
    }
    else{
      return 0;
    }
  }
#else
  char* realpath(const char *path, char *resolved_path);
  char* resolve_path (char* filename){
    return realpath(filename, 0);
  }
#endif

//     Environment Variable Setting
//     ============================
#ifdef PLATFORM_WINDOWS
  int setenv (char* name, char* value, int overwrite) {
    //If we don't want to overwrite previous value, then check whether it exists.
    //If it does, then just return 0.
    if(!overwrite){
      if(getenv(name) == 0)
        return 0;
    }
    //(Over)write the environment variable.
    char* buffer = (char*)malloc(strlen(name) + strlen(value) + 10);
    sprintf(buffer, "%s=%s", name, value);
    _putenv(buffer);
    free(buffer);
    return 0;
  }
#endif

//             Time Modified
//             =============

int64_t file_time_modified (char* filename){
  struct stat attrib;
  if(stat(filename, &attrib) == 0)
    return (int64_t)attrib.st_mtime;
  return 0;
}


//============================================================
//================= Process Runtime ==========================
//============================================================
#if defined(PLATFORM_OS_X) || defined(PLATFORM_LINUX)

//------------------------------------------------------------
//------------------- Structures -----------------------------
//------------------------------------------------------------

typedef struct {
  long pid;
  FILE* in;
  FILE* out;
  FILE* err;
} Process;

typedef struct {
  int state;
  int code;
} ProcessState;

typedef struct {
  char* pipe;
  char* in_pipe;
  char* out_pipe;
  char* err_pipe;
  char* file;
  char** argvs;
} EvalArg;

#define PROCESS_RUNNING 0
#define PROCESS_DONE 1
#define PROCESS_TERMINATED 2
#define PROCESS_STOPPED 3

#define STANDARD_IN 0
#define STANDARD_OUT 1
#define PROCESS_IN 2
#define PROCESS_OUT 3
#define STANDARD_ERR 4
#define PROCESS_ERR 5
#define NUM_STREAM_SPECS 6

//------------------------------------------------------------
//-------------------- Utilities -----------------------------
//------------------------------------------------------------

void exit_with_error (){
  fprintf(stderr, "%s\n", strerror(errno));
  exit(-1);
}

int count_non_null (void** xs){
  int n=0;
  while(xs[n] != NULL)
    n++;
  return n;
}

char* string_join (char* a, char* b){
  int len = strlen(a) + strlen(b);
  char* buffer = (char*)malloc(len + 1);
  sprintf(buffer, "%s%s", a, b);
  return buffer;
}

//Opening a named pipe
int open_pipe (char* prefix, char* suffix, int options){
  char* name = string_join(prefix, suffix);
  int fd = open(name, options);
  if(fd < 0) exit_with_error();
  free(name);
  return fd;
}

//Creating a named pipe
void make_pipe (char* prefix, char* suffix){
  char* name = string_join(prefix, suffix);
  int r = mkfifo(name, S_IRUSR|S_IWUSR);
  if(r < 0) exit_with_error();
}

//Opening a file stream to a pipe
FILE* open_file (int fd, char* mode) {
  FILE* f = fdopen(fd, mode);
  if(f == NULL) exit_with_error();
  return f;
}

//------------------------------------------------------------
//----------------------- Serialization ----------------------
//------------------------------------------------------------

// ===== Serialization =====
void write_int (FILE* f, int x){
  fwrite(&x, sizeof(int), 1, f);
}
void write_long (FILE* f, long x){
  fwrite(&x, sizeof(long), 1, f);
}
void write_string (FILE* f, char* s){
  if(s == NULL)
    write_int(f, -1);
  else{
    int n = strlen(s);
    write_int(f, n);
    fwrite(s, 1, n, f);
  }
}
void write_strings (FILE* f, char** s){
  int n = count_non_null((void**)s);
  write_int(f, n);
  for(int i=0; i<n; i++)
    write_string(f, s[i]);
}
void write_earg (FILE* f, EvalArg* earg){
  write_string(f, earg->pipe);
  write_string(f, earg->in_pipe);
  write_string(f, earg->out_pipe);
  write_string(f, earg->err_pipe);
  write_string(f, earg->file);
  write_strings(f, earg->argvs);
}
void write_process_state (FILE* f, ProcessState* s){
  write_int(f, s->state);
  write_int(f, s->code);
}

// ===== Deserialization =====
void bread (void* xs0, int size, int n0, FILE* f){
  char* xs = xs0;
  int n = n0;
  while(n > 0){
    int c = fread(xs, size, n, f);
    if(c < n){
      if(ferror(f)) exit_with_error();
      if(feof(f)) return;
    }
    n = n - c;
    xs = xs + size*c;
  }
}
int read_int (FILE* f){
  int n;
  bread(&n, sizeof(int), 1, f);
  return n;
}
long read_long (FILE* f){
  long n;
  bread(&n, sizeof(long), 1, f);
  return n;
}
char* read_string (FILE* f){
  int n = read_int(f);
  if(n < 0)
    return NULL;
  else{    
    char* s = (char*)malloc(n + 1);
    bread(s, 1, n, f);
    s[n] = '\0';
    return s;
  }
}
char** read_strings (FILE* f){
  int n = read_int(f);
  char** xs = (char**)malloc(sizeof(char*)*(n + 1));
  for(int i=0; i<n; i++)
    xs[i] = read_string(f);
  xs[n] = NULL;
  return xs;
}
EvalArg* read_earg (FILE* f){
  EvalArg* earg = (EvalArg*)malloc(sizeof(EvalArg));
  earg->pipe = read_string(f);
  earg->in_pipe = read_string(f);
  earg->out_pipe = read_string(f);
  earg->err_pipe = read_string(f);
  earg->file = read_string(f);
  earg->argvs = read_strings(f);
  return earg;
}
void read_process_state (FILE* f, ProcessState* s){  
  s->state = read_int(f);
  s->code = read_int(f);
}

//===== Free =====
void free_earg (EvalArg* arg){
  free(arg->pipe);
  if(arg->in_pipe != NULL) free(arg->in_pipe);
  if(arg->out_pipe != NULL) free(arg->out_pipe);
  if(arg->err_pipe != NULL) free(arg->err_pipe);
  free(arg->file);
  for(int i=0; arg->argvs[i] != NULL; i++)
    free(arg->argvs[i]);
  free(arg->argvs);
}

//------------------------------------------------------------
//-------------------- Process Queries -----------------------
//------------------------------------------------------------

void get_process_state (long pid, ProcessState* s){
  int status;
  int ret = waitpid(pid, &status, WNOHANG);  
  
  if(ret == 0)
    *s = (ProcessState){PROCESS_RUNNING, 0};
  else if(WIFEXITED(status))
    *s = (ProcessState){PROCESS_DONE, WEXITSTATUS(status)};
  else if(WIFSIGNALED(status))
    *s = (ProcessState){PROCESS_TERMINATED, WTERMSIG(status)};
  else if(WIFSTOPPED(status))
    *s = (ProcessState){PROCESS_STOPPED, WSTOPSIG(status)};
  else
    *s = (ProcessState){PROCESS_RUNNING, 0};
}

//------------------------------------------------------------
//---------------------- Launcher Main -----------------------
//------------------------------------------------------------

#define LAUNCH_COMMAND 0
#define STATE_COMMAND 1

void launcher_main (FILE* lin, FILE* lout){
  while(1){
    //Read in command
    int comm = fgetc(lin);
    if(feof(lin)) exit(0);

    //Interpret launch process command
    if(comm == LAUNCH_COMMAND){
      //Read in evaluation arguments
      EvalArg* earg = read_earg(lin);
      if(feof(lin)) exit(0);

      //Create error-code pipe
      int READ = 0;
      int WRITE = 1;
      int exec_error[2];
      if(pipe(exec_error) < 0) exit_with_error();

      //Fork a new child
      long pid = (long)fork();
      if(pid < 0) exit_with_error();

      if(pid > 0){
        //Read from error-code pipe
        int exec_code;
        close(exec_error[WRITE]);
        int exec_r = read(exec_error[READ], &exec_code, sizeof(int));
        close(exec_error[READ]);
        
        if(exec_r == 0){
          //Exec evaluated successfully
          //Return new process id
          write_long(lout, pid);
          fflush(lout);          
        }
        else if(exec_r == sizeof(int)){
          //Exec evaluated unsuccessfully
          //Return error code as negative long
          write_long(lout, -exec_code);
          fflush(lout);
        }
        else{
          fprintf(stderr, "Unreachable code.");
          exit(-1);
        }
      }else{
        //Close exec pipe read, and close write end on successful exec
        close(exec_error[READ]);
        fcntl(exec_error[WRITE], F_SETFD, FD_CLOEXEC);
        
        //Open named pipes
        if(earg->in_pipe != NULL)
          dup2(open_pipe(earg->pipe, earg->in_pipe, O_RDONLY), 0);
        if(earg->out_pipe != NULL)
          dup2(open_pipe(earg->pipe, earg->out_pipe, O_WRONLY), 1);
        if(earg->err_pipe != NULL)
          dup2(open_pipe(earg->pipe, earg->err_pipe, O_WRONLY), 2);
        
        //Launch child process      
        execvp(earg->file, earg->argvs);

        //Unsuccessful exec, write error number
        int error_code = errno;
        write(exec_error[WRITE], &error_code, sizeof(int));
        close(exec_error[WRITE]);
      }
    }
    //Interpret state retrieval command
    else if(comm == STATE_COMMAND){
      //Read in process id
      long pid = read_long(lin);

      //Retrieve state
      ProcessState s; get_process_state(pid, &s);
      write_process_state(lout, &s);
      fflush(lout);
    }
    //Unrecognized command
    else{
      fprintf(stderr, "Illegal command: %d\n", comm);
      exit(-1);
    }
  }
}

long launcher_pid = -1;
FILE* launcher_in = NULL;
FILE* launcher_out = NULL;
void initialize_launcher_process (){
  if(launcher_pid < 0){    
    //Create pipes
    int READ = 0;
    int WRITE = 1;
    int lin[2];
    int lout[2];
    if(pipe(lin) < 0) exit_with_error();
    if(pipe(lout) < 0) exit_with_error();

    //Fork
    long pid = (long)fork();
    if(pid < 0) exit_with_error();

    if(pid > 0){
      //Parent
      launcher_pid = pid;
      close(lin[READ]);
      close(lout[WRITE]);
      launcher_in = fdopen(lin[WRITE], "w");
      if(launcher_in == NULL) exit_with_error();
      launcher_out = fdopen(lout[READ], "r");
      if(launcher_out == NULL) exit_with_error();
    }
    else{
      //Child
      close(lin[WRITE]);
      close(lout[READ]);
      FILE* fin = fdopen(lin[READ], "r");
      if(fin == NULL) exit_with_error();
      FILE* fout = fdopen(lout[WRITE], "w");
      if(fout == NULL) exit_with_error();
      launcher_main(fin, fout);
    }
  }
}

int launch_process (char* file, char** argvs,
                    int input, int output, int error,
                    Process* process){
  //Initialize launcher if necessary
  initialize_launcher_process();
  
  //Figure out unique pipe name
  char pipe_name[80];
  sprintf(pipe_name, "/tmp/stanza_exec_pipe%ld", (long)getpid());

  //Compute pipe sources
  int pipe_sources[NUM_STREAM_SPECS];
  for(int i=0; i<NUM_STREAM_SPECS; i++)
    pipe_sources[i] = -1;
  pipe_sources[input] = 0;
  pipe_sources[output] = 1;
  pipe_sources[error] = 2;
  
  //Create pipes to child
  if(pipe_sources[PROCESS_IN] >= 0)
    make_pipe(pipe_name, "_in");
  if(pipe_sources[PROCESS_OUT] >= 0)
    make_pipe(pipe_name, "_out");
  if(pipe_sources[PROCESS_ERR] >= 0)
    make_pipe(pipe_name, "_err");
  
  //Write in command and evaluation arguments
  EvalArg earg = {pipe_name, NULL, NULL, NULL, file, argvs};
  if(input == PROCESS_IN) earg.in_pipe = "_in";
  if(output == PROCESS_OUT) earg.out_pipe = "_out";
  if(output == PROCESS_ERR) earg.out_pipe = "_err";
  if(error == PROCESS_OUT) earg.err_pipe = "_out";
  if(error == PROCESS_ERR) earg.err_pipe = "_err";
  int r = fputc(LAUNCH_COMMAND, launcher_in);
  if(r == EOF) exit_with_error();
  write_earg(launcher_in, &earg);
  fflush(launcher_in);
  
  //Read back process id, and set errno if failed
  long pid = read_long(launcher_out);
  if(pid < 0){
    errno = (int)(- pid);
    return -1;
  }
  
  //Open pipes to child
  FILE* fin = NULL;
  if(pipe_sources[PROCESS_IN] >= 0)
    fin = open_file(open_pipe(pipe_name, "_in", O_WRONLY), "w");
  FILE* fout = NULL;
  if(pipe_sources[PROCESS_OUT] >= 0)
    fout = open_file(open_pipe(pipe_name, "_out", O_RDONLY), "r");
  FILE* ferr = NULL;
  if(pipe_sources[PROCESS_ERR] >= 0)
    ferr = open_file(open_pipe(pipe_name, "_err", O_RDONLY), "r");
  
  //Return process structure
  process->pid = pid;
  process->in = fin;
  process->out = fout;
  process->err = ferr;
  return 0;
}

void retrieve_process_state (long pid, ProcessState* s){
  //Check whether launcher has been initialized
  if(launcher_pid < 0){
    fprintf(stderr, "Launcher not initialized.\n");
    exit(-1);
  }
    
  //Send command
  int r = fputc(STATE_COMMAND, launcher_in);
  if(r == EOF) exit_with_error();
  write_long(launcher_in, pid);
  fflush(launcher_in);

  //Read back process state
  read_process_state(launcher_out, s);
}

#endif
//============================================================
//============== End Process Runtime =========================
//============================================================
