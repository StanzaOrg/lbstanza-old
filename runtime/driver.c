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
#include<sys/mman.h>
#include<dirent.h>
#include<pthread.h>

//       Forward Declarations
//       ====================

//FMalloc Debugging
void init_fmalloc ();

//Stanza Alloc
void* stz_malloc (long size);
void stz_free (void* ptr);

//     Stanza Defined Entities
//     =======================
typedef struct{
  char* heap;
  char* heap_top;
  char* heap_limit;
  char* free;
  char* free_limit;
  uint64_t current_stack;  
  uint64_t system_stack;  
} VMInit;

typedef struct{
  uint64_t returnpc;
  uint64_t liveness_map;
  uint64_t slots[];
} StackFrame;

typedef struct{
  int pool_index;
  int mark;
  StackFrame frames[];
} StackFrameHeader;

typedef struct{
  uint64_t size;
  StackFrame* frames;
  StackFrame* stack_pointer;
  int pc;
} Stack;

#define STACK_TYPE 6

int64_t stanza_entry (VMInit* init);

//     Command line arguments
//     ======================
int input_argc;
char** input_argv;
int input_argv_needs_free;

//     Main Driver
//     ===========
void* alloc (VMInit* init, long type, long size){
  void* ptr = init->heap_top + 8;
  *(long*)(init->heap_top) = type;
  init->heap_top += 8 + size;
  return ptr;  
}

uint64_t alloc_stack (VMInit* init){
  Stack* stack = alloc(init, STACK_TYPE, sizeof(Stack));
  int initial_stack_size = 4 * 1024;
  long size = initial_stack_size + sizeof(StackFrameHeader);
  StackFrameHeader* frameheader = (StackFrameHeader*)stz_malloc(size);
  frameheader->pool_index = -1;
  frameheader->mark = 0;
  stack->size = initial_stack_size;
  stack->frames = frameheader->frames;
  stack->stack_pointer = NULL;
  return (uint64_t)stack - 8 + 1;  
}

int main (int argc, char* argv[]) {
  #if defined(FMALLOC)
    init_fmalloc();
  #endif
  
  input_argc = argc;
  input_argv = argv;
  input_argv_needs_free = 0;
  VMInit init;

  //Allocate heap and free
  int initial_heap_size = 1024 * 1024;
  init.heap = (char*)stz_malloc(initial_heap_size);
  init.heap_limit = init.heap + initial_heap_size;
  init.heap_top = init.heap;
  init.free = (char*)stz_malloc(initial_heap_size);
  init.free_limit = init.free + initial_heap_size;

  //Allocate stacks
  init.current_stack = alloc_stack(&init);
  init.system_stack = alloc_stack(&init);   

  //Call Stanza entry
  stanza_entry(&init);
  
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
      char* path = (char*)stz_malloc(2048);
      int ret = GetFullPathName(filename, 2048, path, &fileext);
      if(ret == 0){
        stz_free(path);
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
    char* buffer = (char*)stz_malloc(strlen(name) + strlen(value) + 10);
    sprintf(buffer, "%s=%s", name, value);
    int r = _putenv(buffer);
    stz_free(buffer);
    return r;
  }

  int unsetenv (char* name){
    char* buffer = (char*)stz_malloc(strlen(name) + 10);
    sprintf(buffer, "%s=", name);
    int r = _putenv(buffer);
    stz_free(buffer);
    return r;
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
//======================= Free List ==========================
//============================================================

typedef struct {
  int capacity;
  int size;
  void** items;
} FreeList;

FreeList make_freelist (int c){
  void** items = (void**)malloc(c * sizeof(void*));
  FreeList f = {c, 0, items};
  return f;
}

void ensure_capacity (FreeList* list, int c){
  if(list->capacity < c){
    int c2 = list->capacity;
    while(c2 < c) c2 *= 2;
    void** items2 = (void**)malloc(c2 * sizeof(void*));
    memcpy(items2, list->items, list->capacity * sizeof(void*));
    free(list->items);
    list->items = items2;
    list->capacity = c2;
  }
}

void delete_index (FreeList* list, int xi){
  int yi = list->size - 1;
  void* x = list->items[xi];
  void* y = list->items[yi];
  if(xi != yi){
    list->items[xi] = y;
    list->items[yi] = x;
  }
  list->size--;
}

void add_item (FreeList* list, void* item){
  int i = list->size;
  ensure_capacity(list, i + 1);
  list->items[i] = item;
  list->size++;
}

//============================================================
//================== Fixed Memory Allocator ==================
//============================================================

typedef struct {
  long size;
  char bytes[];
} Chunk;

char* mem_top;
char* mem_limit;
FreeList mem_chunks;

void init_fmalloc () {
  mem_chunks = make_freelist(8);
  long size = 8L * 1024L * 1024L * 1024L;
  mem_top = (char*)0x700000000L;
  mem_limit = mem_top + size;
  void* result = mmap(mem_top,
                      size,            
                      PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED,
                      0,
                      0);
  if(!result){
    printf("Could not allocate fixed memory.\n");
    exit(-1);
  }  
}

Chunk* alloc_chunk (long size){  
  long total_size = (size + sizeof(Chunk) + 7) & -8;
  Chunk* chunk = (Chunk*)mem_top;
  mem_top += total_size;
  if(mem_top > mem_limit){
    printf("Out of fixed memory.\n");
    exit(-1);
  }
  chunk->size = size;
  return chunk;
}

Chunk* find_chunk (long size){
  Chunk* best = 0;
  int besti = 0;
  for(int i=0; i<mem_chunks.size; i++){
    Chunk* c = mem_chunks.items[i];
    int is_best = 0;
    if(c->size >= size){
      if(best) is_best = c->size < best->size;
      else is_best = 1;
    }
    if(is_best){
      best = c;
      besti = i;
    }
  }
  if(best)
    delete_index(&mem_chunks, besti);
  return best;
}

void* fmalloc (long size){
  Chunk* c = find_chunk(size);
  if(!c) c = alloc_chunk(size);
  return c->bytes;
}

void ffree (void* ptr){
  Chunk* c = (Chunk*)((char*)ptr - sizeof(Chunk));
  add_item(&mem_chunks, c);
}

//============================================================
//===================== String List ==========================
//============================================================

typedef struct {
  int n;
  int capacity;
  char** strings;
} StringList;

StringList* make_stringlist (int capacity){
  StringList* list = (StringList*)malloc(sizeof(StringList));
  list->n = 0;
  list->capacity = capacity;
  list->strings = malloc(capacity * sizeof(char*));
  return list;
}

void ensure_stringlist_capacity (StringList* list, int c) {
  if(list->capacity < c){
    int new_capacity = list->capacity;
    while(new_capacity < c) new_capacity *= 2;
    char** new_strings = malloc(new_capacity * sizeof(char*));
    memcpy(new_strings, list->strings, list->n * sizeof(char*));
    list->capacity = new_capacity;
    free(list->strings);
    list->strings = new_strings;
  }
}

void free_stringlist (StringList* list){
  for(int i=0; i<list->n; i++)
    free(list->strings[i]);
  free(list->strings);
  free(list);
}

void stringlist_add (StringList* list, char* string){
  ensure_stringlist_capacity(list, list->n + 1);
  char* copy = malloc(strlen(string) + 1);
  strcpy(copy, string);
  list->strings[list->n] = copy;
  list->n++;
}

//============================================================
//================== Directory Handling ======================
//============================================================

int get_file_type (char* filename) {
  struct stat filestat;
  if(stat(filename, &filestat) == 0){
    if(S_ISREG(filestat.st_mode))
      return 0;
    else if(S_ISDIR(filestat.st_mode))
      return 1;
    else
      return 2;    
  }
  else{
    return -1;
  }
}

StringList* list_dir (char* filename){
  //Open directory
  DIR* dir = opendir(filename);
  if(dir == NULL) return 0;
  
  //Allocate memory for strings
  StringList* list = make_stringlist(10);
  //Loop through directory entries
  while(1){
    //Read next entry
    struct dirent* entry = readdir(dir);
    if(entry == NULL){
      closedir(dir);
      return list;
    }
    //Notify
    stringlist_add(list, entry->d_name);
  }

  free(list);
  return 0;
}

//============================================================
//===================== Sleeping =============================
//============================================================

int sleep_us (long us){
  struct timespec t1, t2;
  t1.tv_sec = 0;
  t1.tv_nsec = us * 1000L;
  return nanosleep(&t1, &t2);
}

//============================================================
//================= Stanza Memory Allocator ==================
//============================================================

void* stz_malloc (long size){
  #if defined(FMALLOC)
    return fmalloc(size);
  #else
    return malloc(size);
  #endif
}

void stz_free (void* ptr){
  #if defined(FMALLOC)
    ffree(ptr);
  #else
    free(ptr);
  #endif
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
  int pipeid;
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

#define RETURN_NEG(x) {int r=(x); if(r < 0) return -1;}

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
  char* buffer = (char*)stz_malloc(len + 1);
  sprintf(buffer, "%s%s", a, b);
  return buffer;
}

//Opening a named pipe
int open_pipe (char* prefix, char* suffix, int options){
  char* name = string_join(prefix, suffix);
  int fd = open(name, options);
  stz_free(name);
  return fd;
}

//Creating a named pipe
int make_pipe (char* prefix, char* suffix){
  char* name = string_join(prefix, suffix);
  return mkfifo(name, S_IRUSR|S_IWUSR);
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
    char* s = (char*)stz_malloc(n + 1);
    bread(s, 1, n, f);
    s[n] = '\0';
    return s;
  }
}
char** read_strings (FILE* f){
  int n = read_int(f);
  char** xs = (char**)stz_malloc(sizeof(char*)*(n + 1));
  for(int i=0; i<n; i++)
    xs[i] = read_string(f);
  xs[n] = NULL;
  return xs;
}
EvalArg* read_earg (FILE* f){
  EvalArg* earg = (EvalArg*)stz_malloc(sizeof(EvalArg));
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
  stz_free(arg->pipe);
  if(arg->in_pipe != NULL) stz_free(arg->in_pipe);
  if(arg->out_pipe != NULL) stz_free(arg->out_pipe);
  if(arg->err_pipe != NULL) stz_free(arg->err_pipe);
  stz_free(arg->file);
  for(int i=0; arg->argvs[i] != NULL; i++)
    stz_free(arg->argvs[i]);
  stz_free(arg->argvs);
}

//------------------------------------------------------------
//-------------------- Process Queries -----------------------
//------------------------------------------------------------

void get_process_state (long pid, ProcessState* s, int wait_for_termination){
  int status;
  int ret = waitpid(pid, &status, wait_for_termination? 0 : WNOHANG);  
  
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
#define WAIT_COMMAND 2

void write_error_and_exit (int fd){
  int code = errno;
  write(fd, &code, sizeof(int));
  close(fd);
  exit(-1);
}

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
        if(earg->in_pipe != NULL){
          int fd = open_pipe(earg->pipe, earg->in_pipe, O_RDONLY);
          if(fd < 0) write_error_and_exit(exec_error[WRITE]);
          dup2(fd, 0);
        }
        if(earg->out_pipe != NULL){
          int fd = open_pipe(earg->pipe, earg->out_pipe, O_WRONLY);
          if(fd < 0) write_error_and_exit(exec_error[WRITE]);
          dup2(fd, 1);
        }
        if(earg->err_pipe != NULL){
          int fd = open_pipe(earg->pipe, earg->err_pipe, O_WRONLY);
          if(fd < 0) write_error_and_exit(exec_error[WRITE]);
          dup2(fd, 2);
        }
        
        //Launch child process      
        execvp(earg->file, earg->argvs);

        //Unsuccessful exec, write error number
        write_error_and_exit(exec_error[WRITE]);
      }
    }
    //Interpret state retrieval command
    else if(comm == STATE_COMMAND || comm == WAIT_COMMAND){
      //Read in process id
      long pid = read_long(lin);

      //Retrieve state
      ProcessState s; get_process_state(pid, &s, comm == WAIT_COMMAND);
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

void make_pipe_name (char* pipe_name, int pipeid) {
  sprintf(pipe_name, "/tmp/stanza_exec_pipe_%ld_%ld", (long)getpid(), (long)pipeid);
}

int delete_process_pipe (FILE* fd, char* pipe_name, char* suffix) {
  int close_res = fclose(fd);
  if (close_res == EOF) return -1;
  char my_pipe_name[80];
  sprintf(my_pipe_name, "%s%s", pipe_name, suffix);
  int rm_res = remove(my_pipe_name);
  if (rm_res < 0) return -1;
  return 0;
}

int delete_process_pipes (FILE* input, FILE* output, FILE* error, int pipeid) {
  char pipe_name[80];
  make_pipe_name(pipe_name, pipeid);
  if (delete_process_pipe(input,  pipe_name, "_in") < 0)
    return -1;
  if (delete_process_pipe(output, pipe_name, "_out") < 0)
    return -1;
  if (delete_process_pipe(error,  pipe_name, "_err") < 0)
    return -1;
  return 0;
}

int launch_process (char* file, char** argvs,
                    int input, int output, int error, int pipeid,
                    Process* process){
  //Initialize launcher if necessary
  initialize_launcher_process();
  
  //Figure out unique pipe name
  char pipe_name[80];
  make_pipe_name(pipe_name, pipeid);

  //Compute pipe sources
  int pipe_sources[NUM_STREAM_SPECS];
  for(int i=0; i<NUM_STREAM_SPECS; i++)
    pipe_sources[i] = -1;
  pipe_sources[input] = 0;
  pipe_sources[output] = 1;
  pipe_sources[error] = 2;
  
  //Create pipes to child
  if(pipe_sources[PROCESS_IN] >= 0)
    RETURN_NEG(make_pipe(pipe_name, "_in"))
  if(pipe_sources[PROCESS_OUT] >= 0)
    RETURN_NEG(make_pipe(pipe_name, "_out"))
  if(pipe_sources[PROCESS_ERR] >= 0)
    RETURN_NEG(make_pipe(pipe_name, "_err"))
  
  //Write in command and evaluation arguments
  EvalArg earg = {pipe_name, NULL, NULL, NULL, file, argvs};
  if(input == PROCESS_IN) earg.in_pipe = "_in";
  if(output == PROCESS_OUT) earg.out_pipe = "_out";
  if(output == PROCESS_ERR) earg.out_pipe = "_err";
  if(error == PROCESS_OUT) earg.err_pipe = "_out";
  if(error == PROCESS_ERR) earg.err_pipe = "_err";
  int r = fputc(LAUNCH_COMMAND, launcher_in);
  if(r == EOF) return -1;
  write_earg(launcher_in, &earg);
  fflush(launcher_in);

  //Open pipes to child
  FILE* fin = NULL;
  if(pipe_sources[PROCESS_IN] >= 0){
    int fd = open_pipe(pipe_name, "_in", O_WRONLY);
    RETURN_NEG(fd)
    fin = fdopen(fd, "w");
    if(fin == NULL) return -1;
  }
  FILE* fout = NULL;
  if(pipe_sources[PROCESS_OUT] >= 0){
    int fd = open_pipe(pipe_name, "_out", O_RDONLY);
    RETURN_NEG(fd)
    fout = fdopen(fd, "r");
    if(fout == NULL) return -1;
  }
  FILE* ferr = NULL;
  if(pipe_sources[PROCESS_ERR] >= 0){
    int fd = open_pipe(pipe_name, "_err", O_RDONLY);
    RETURN_NEG(fd)
    ferr = fdopen(fd, "r");
    if(ferr == NULL) return -1;
  }
  
  //Read back process id, and set errno if failed
  long pid = read_long(launcher_out);
  if(pid < 0){
    errno = (int)(- pid);
    return -1;
  } 
  
  //Return process structure
  process->pid = pid;
  process->in = fin;
  process->out = fout;
  process->err = ferr;
  return 0;
}

void retrieve_process_state (long pid, ProcessState* s, int wait_for_termination){
  //Check whether launcher has been initialized
  if(launcher_pid < 0){
    fprintf(stderr, "Launcher not initialized.\n");
    exit(-1);
  }
    
  //Send command
  int r = fputc(wait_for_termination? WAIT_COMMAND : STATE_COMMAND, launcher_in);
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
