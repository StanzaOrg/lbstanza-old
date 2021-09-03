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

#include "common.h"
#include "types.h"

//       Forward Declarations
//       ====================

//FMalloc Debugging
static void init_fmalloc ();

void* stz_malloc (stz_long size);
void stz_free (void* ptr);

//     Stanza Defined Entities
//     =======================
typedef struct{
  stz_byte* heap;
  stz_byte* heap_top;
  stz_byte* heap_limit;
  stz_byte* free;
  stz_byte* free_limit;
  stz_long current_stack;
  stz_long system_stack;
} VMInit;

typedef struct{
  stz_long returnpc;
  stz_long liveness_map;
  stz_long slots[];
} StackFrame;

typedef struct{
  stz_int pool_index;
  stz_int mark;
  StackFrame frames[];
} StackFrameHeader;

typedef struct{
  stz_long size;
  StackFrame* frames;
  StackFrame* stack_pointer;
  stz_long pc;
} Stack;

//     Macro Readers
//     =============
FILE* get_stdout () {return stdout;}
FILE* get_stderr () {return stderr;}
FILE* get_stdin () {return stdin;}
stz_int get_eof () {return (stz_int)EOF;}
stz_int get_errno () {return (stz_int)errno;}

//     Time of Day
//     ===========
stz_long current_time_us (void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (stz_long)tv.tv_sec * 1000 * 1000 + (stz_long)tv.tv_usec;
}

stz_long current_time_ms (void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (stz_long)tv.tv_sec * 1000 + (stz_long)tv.tv_usec / 1000;
}

//     Random Access Files
//     ===================
stz_long get_file_size (FILE* f) {
  int64_t cur_pos = ftell(f);
  fseek(f, 0, SEEK_END);
  stz_long size = (stz_long)ftell(f);
  fseek(f, cur_pos, SEEK_SET);
  return size;
}

stz_int file_seek (FILE* f, stz_long pos) {
  return (stz_int)fseek(f, pos, SEEK_SET);
}

stz_int file_skip (FILE* f, stz_long num) {
  return (stz_int)fseek(f, num, SEEK_CUR);
}

stz_int file_set_length (FILE* f, stz_long size) {
  return (stz_int)ftruncate(fileno(f), size);
}

stz_long file_read_block (FILE* f, char* data, stz_long len) {
  return (stz_long)fread(data, 1, len, f);
}

stz_long file_write_block (FILE* f, char* data, stz_long len) {
  return (stz_long)fwrite(data, 1, len, f);
}


//     Path Resolution
//     ===============
#ifdef PLATFORM_WINDOWS
  static int file_exists (const stz_byte* filename) {
    int attrib = GetFileAttributes((LPCSTR)filename);
    return attrib != INVALID_FILE_ATTRIBUTES;
  }

  stz_byte* resolve_path (const stz_byte* filename){
    if(file_exists(filename)){
      char* fileext;
      char* path = (char*)stz_malloc(2048);
      int ret = GetFullPathName((LPCSTR)filename, 2048, path, &fileext);
      if(ret == 0){
        stz_free(path);
        return NULL;
      }else{
        return STZ_STR(path);
      }
    }
    else{
      return NULL;
    }
  }
#else
  stz_byte* resolve_path (const stz_byte* filename){
    return STZ_STR(realpath(C_CSTR(filename), 0));
  }
#endif

//     Environment Variable Setting
//     ============================
#ifdef PLATFORM_WINDOWS
  stz_int setenv (const stz_byte* name, const stz_byte* value, stz_int overwrite) {
    //If we don't want to overwrite previous value, then check whether it exists.
    //If it does, then just return 0.
    if(!overwrite){
      if(getenv(C_CSTR(name)) == 0)
        return 0;
    }
    //(Over)write the environment variable.
    char* buffer = (char*)stz_malloc(strlen(C_CSTR(name)) + strlen(C_CSTR(value)) + 10);
    sprintf(buffer, "%s=%s", C_CSTR(name), C_CSTR(value));
    int r = _putenv(buffer);
    stz_free(buffer);
    return (stz_int)r;
  }

  stz_int unsetenv (const stz_byte* name){
    char* buffer = (char*)stz_malloc(strlen(C_CSTR(name)) + 10);
    sprintf(buffer, "%s=", C_CSTR(name));
    int r = _putenv(buffer);
    stz_free(buffer);
    return (stz_int)r;
  }
#endif

//             Time Modified
//             =============

stz_long file_time_modified (const stz_byte* filename){
  struct stat attrib;
  if(stat(C_CSTR(filename), &attrib) == 0)
    return (stz_long)attrib.st_mtime;
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

static FreeList make_freelist (int c){
  void** items = (void**)malloc(c * sizeof(void*));
  FreeList f = {c, 0, items};
  return f;
}

static void ensure_capacity (FreeList* list, int c){
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

static void delete_index (FreeList* list, int xi){
  int yi = list->size - 1;
  void* x = list->items[xi];
  void* y = list->items[yi];
  if(xi != yi){
    list->items[xi] = y;
    list->items[yi] = x;
  }
  list->size--;
}

static void add_item (FreeList* list, void* item){
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

static void init_fmalloc () {
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

static Chunk* alloc_chunk (long size){
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

static Chunk* find_chunk (long size){
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

static void* fmalloc (long size){
  Chunk* c = find_chunk(size);
  if(!c) c = alloc_chunk(size);
  return c->bytes;
}

static void ffree (void* ptr){
  Chunk* c = (Chunk*)((char*)ptr - sizeof(Chunk));
  add_item(&mem_chunks, c);
}

//============================================================
//===================== String List ==========================
//============================================================

typedef struct {
  stz_int n;
  stz_int capacity;
  stz_byte** strings;
} StringList;

StringList* make_stringlist (stz_int capacity){
  StringList* list = (StringList*)malloc(sizeof(StringList));
  list->n = 0;
  list->capacity = capacity;
  list->strings = (stz_byte**)malloc(capacity * sizeof(stz_byte*));
  return list;
}

static void ensure_stringlist_capacity (StringList* list, stz_int c) {
  if(list->capacity < c){
    stz_int new_capacity = list->capacity;
    while(new_capacity < c) new_capacity *= 2;
    stz_byte** new_strings = (stz_byte**)malloc(new_capacity * sizeof(stz_byte*));
    memcpy(new_strings, list->strings, list->n * sizeof(stz_byte*));
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

void stringlist_add (StringList* list, const stz_byte* string){
  ensure_stringlist_capacity(list, list->n + 1);
  char* copy = malloc(strlen(C_CSTR(string)) + 1);
  strcpy(copy, C_CSTR(string));
  list->strings[list->n] = STZ_STR(copy);
  list->n++;
}

//============================================================
//================== Directory Handling ======================
//============================================================

stz_int get_file_type (const stz_byte* filename, stz_int follow_sym_links) {
  struct stat filestat;  
  int result;
  if(follow_sym_links) result = stat(C_CSTR(filename), &filestat);
  else result = lstat(C_CSTR(filename), &filestat);
                         
  if(result == 0){
    if(S_ISREG(filestat.st_mode))
      return 0;
    else if(S_ISDIR(filestat.st_mode))
      return 1;
    else if(S_ISLNK(filestat.st_mode))
      return 2;
    else
      return 3;    
  }
  else{
    return -1;
  }
}

StringList* list_dir (const stz_byte* filename){
  //Open directory
  DIR* dir = opendir(C_CSTR(filename));
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
    stringlist_add(list, STZ_CSTR(entry->d_name));
  }

  free(list);
  return 0;
}

//============================================================
//===================== Sleeping =============================
//============================================================

stz_int sleep_us (stz_long us){
  struct timespec t1, t2;
  t1.tv_sec = 0;
  t1.tv_nsec = us * 1000L;
  return (stz_int)nanosleep(&t1, &t2);
}

//============================================================
//================= Stanza Memory Allocator ==================
//============================================================

void* stz_malloc (stz_long size){
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
  stz_byte* pipe;
  stz_byte* in_pipe;
  stz_byte* out_pipe;
  stz_byte* err_pipe;
  stz_byte* file;
  stz_byte** argvs;
} EvalArg;

#define RETURN_NEG(x) {int r=(x); if(r < 0) return -1;}

//------------------------------------------------------------
//-------------------- Utilities -----------------------------
//------------------------------------------------------------

static void exit_with_error (){
  fprintf(stderr, "%s\n", strerror(errno));
  exit(-1);
}

static int count_non_null (void** xs){
  int n=0;
  while(xs[n] != NULL)
    n++;
  return n;
}

static char* string_join (const char* a, const char* b){
  int len = strlen(a) + strlen(b);
  char* buffer = (char*)stz_malloc(len + 1);
  sprintf(buffer, "%s%s", a, b);
  return buffer;
}

//Opening a named pipe
static int open_pipe (const char* prefix, const char* suffix, int options){
  char* name = string_join(prefix, suffix);
  int fd = open(name, options);
  stz_free(name);
  return fd;
}

//Creating a named pipe
static int make_pipe (char* prefix, char* suffix){
  char* name = string_join(prefix, suffix);
  return mkfifo(name, S_IRUSR|S_IWUSR);
}

//============================================================
//================== Stanza Memory Mapping ===================
//============================================================

//Set protection bits on address range p (inclusive) to p + size (exclusive).
//Fatal error if size > 0 and mprotect fails.
static void protect(void* p, stz_long size, stz_int prot) {
  if (size && mprotect(p, (size_t)size, prot)) exit_with_error();
}

//Allocates a segment of memory that is min_size allocated, and can be
//resized up to max_size.
//This function is called from within Stanza, and min_size and max_size
//are assumed to be multiples of the system page size.
void* stz_memory_map (stz_long min_size, stz_long max_size) {
  void* p = mmap(NULL, (size_t)max_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED) exit_with_error();

  protect(p, min_size, PROT_READ | PROT_WRITE | PROT_EXEC);
  return p;
}

//Unmaps the region of mememory. 
//This function is called from within Stanza, and size is 
//assumed to be a multiple of the system page size.
void stz_memory_unmap (void* p, stz_long size) {
  if (p && munmap(p, (size_t)size)) exit_with_error();
}

//Resizes the given segment.
//old_size is assumed to be the size that is already allocated.
//new_size is the size that we desired to be allocated, and
//must be a multiple of the system page size.
void stz_memory_resize (void* p, stz_long old_size, stz_long new_size) {
  stz_long min_size = old_size;
  stz_long max_size = new_size;
  int prot = PROT_READ | PROT_WRITE | PROT_EXEC;

  if (min_size > max_size) {
    min_size = new_size;
    max_size = old_size;
    prot = PROT_NONE;
  }

  protect((char*)p + min_size, max_size - min_size, prot);
}

//------------------------------------------------------------
//----------------------- Serialization ----------------------
//------------------------------------------------------------

// ===== Serialization =====
static void write_int (FILE* f, stz_int x){
  fwrite(&x, sizeof(stz_int), 1, f);
}
static void write_long (FILE* f, stz_long x){
  fwrite(&x, sizeof(stz_long), 1, f);
}
static void write_string (FILE* f, stz_byte* s){
  if(s == NULL)
    write_int(f, -1);
  else{
    size_t n = strlen(C_CSTR(s));
    write_int(f, (stz_int)n);
    fwrite(s, 1, n, f);
  }
}
static void write_strings (FILE* f, stz_byte** s){
  int n = count_non_null((void**)s);
  write_int(f, (stz_int)n);
  for(int i=0; i<n; i++)
    write_string(f, s[i]);
}
static void write_earg (FILE* f, EvalArg* earg){
  write_string(f, earg->pipe);
  write_string(f, earg->in_pipe);
  write_string(f, earg->out_pipe);
  write_string(f, earg->err_pipe);
  write_string(f, earg->file);
  write_strings(f, earg->argvs);
}
static void write_process_state (FILE* f, ProcessState* s){
  write_int(f, s->state);
  write_int(f, s->code);
}

// ===== Deserialization =====
static void bread (void* xs0, int size, int n0, FILE* f){
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
static stz_int read_int (FILE* f){
  stz_int n;
  bread(&n, sizeof(stz_int), 1, f);
  return n;
}
static stz_long read_long (FILE* f){
  stz_long n;
  bread(&n, sizeof(stz_long), 1, f);
  return n;
}
static stz_byte* read_string (FILE* f){
  stz_int n = read_int(f);
  if(n < 0)
    return NULL;
  else{    
    stz_byte* s = (stz_byte*)stz_malloc(n + 1);
    bread(s, 1, (int)n, f);
    s[n] = '\0';
    return s;
  }
}
static stz_byte** read_strings (FILE* f){
  stz_int n = read_int(f);
  stz_byte** xs = (stz_byte**)stz_malloc(sizeof(stz_byte*)*(n + 1));
  for(int i=0; i<n; i++)
    xs[i] = read_string(f);
  xs[n] = NULL;
  return xs;
}
static EvalArg* read_earg (FILE* f){
  EvalArg* earg = (EvalArg*)stz_malloc(sizeof(EvalArg));
  earg->pipe = read_string(f);
  earg->in_pipe = read_string(f);
  earg->out_pipe = read_string(f);
  earg->err_pipe = read_string(f);
  earg->file = read_string(f);
  earg->argvs = read_strings(f);
  return earg;
}
static void read_process_state (FILE* f, ProcessState* s){
  s->state = read_int(f);
  s->code = read_int(f);
}

//===== Free =====
static void free_earg (EvalArg* arg){
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

static void get_process_state (stz_long pid, ProcessState* s, int wait_for_termination){
  int status;
  int ret = waitpid((pid_t)pid, &status, wait_for_termination? 0 : WNOHANG);
  
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

static void write_error_and_exit (int fd){
  int code = errno;
  write(fd, &code, sizeof(int));
  close(fd);
  exit(-1);
}

static void launcher_main (FILE* lin, FILE* lout){
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
      stz_long pid = (stz_long)fork();
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
          write_long(lout, (stz_long)-exec_code);
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
          int fd = open_pipe(C_CSTR(earg->pipe), C_CSTR(earg->in_pipe), O_RDONLY);
          if(fd < 0) write_error_and_exit(exec_error[WRITE]);
          dup2(fd, 0);
        }
        if(earg->out_pipe != NULL){
          int fd = open_pipe(C_CSTR(earg->pipe), C_CSTR(earg->out_pipe), O_WRONLY);
          if(fd < 0) write_error_and_exit(exec_error[WRITE]);
          dup2(fd, 1);
        }
        if(earg->err_pipe != NULL){
          int fd = open_pipe(C_CSTR(earg->pipe), C_CSTR(earg->err_pipe), O_WRONLY);
          if(fd < 0) write_error_and_exit(exec_error[WRITE]);
          dup2(fd, 2);
        }
        
        //Launch child process      
        execvp(C_CSTR(earg->file), (char**)earg->argvs);

        //Unsuccessful exec, write error number
        write_error_and_exit(exec_error[WRITE]);
      }
    }
    //Interpret state retrieval command
    else if(comm == STATE_COMMAND || comm == WAIT_COMMAND){
      //Read in process id
      stz_long pid = read_long(lin);

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

static stz_long launcher_pid = -1;
static FILE* launcher_in = NULL;
static FILE* launcher_out = NULL;
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
    stz_long pid = (stz_long)fork();
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

static void make_pipe_name (char* pipe_name, int pipeid) {
  sprintf(pipe_name, "/tmp/stanza_exec_pipe_%ld_%ld", (long)getpid(), (long)pipeid);
}

static int delete_process_pipe (FILE* fd, char* pipe_name, char* suffix) {
  if (fd != NULL) {
    int close_res = fclose(fd);
    if (close_res == EOF) return -1;
    char my_pipe_name[80];
    sprintf(my_pipe_name, "%s%s", pipe_name, suffix);
    int rm_res = remove(my_pipe_name);
    if (rm_res < 0) return -1;
  }
  return 0;
}

stz_int delete_process_pipes (FILE* input, FILE* output, FILE* error, stz_int pipeid) {
  char pipe_name[80];
  make_pipe_name(pipe_name, (int)pipeid);
  if (delete_process_pipe(input,  pipe_name, "_in") < 0)
    return -1;
  if (delete_process_pipe(output, pipe_name, "_out") < 0)
    return -1;
  if (delete_process_pipe(error,  pipe_name, "_err") < 0)
    return -1;
  return 0;
}

stz_int launch_process(stz_byte* file, stz_byte** argvs, stz_int input,
                       stz_int output, stz_int error, stz_int pipeid,
                       Process* process) {
  //Initialize launcher if necessary
  initialize_launcher_process();
  
  //Figure out unique pipe name
  char pipe_name[80];
  make_pipe_name(pipe_name, (int)pipeid);

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
  EvalArg earg = {STZ_STR(pipe_name), NULL, NULL, NULL, file, argvs};
  if(input == PROCESS_IN) earg.in_pipe = STZ_STR("_in");
  if(output == PROCESS_OUT) earg.out_pipe = STZ_STR("_out");
  if(output == PROCESS_ERR) earg.out_pipe = STZ_STR("_err");
  if(error == PROCESS_OUT) earg.err_pipe = STZ_STR("_out");
  if(error == PROCESS_ERR) earg.err_pipe = STZ_STR("_err");
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
  stz_long pid = read_long(launcher_out);
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

void retrieve_process_state (stz_long pid, ProcessState* s, stz_int wait_for_termination){
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
#else
#include "process-win32.c"
//============================================================
//============== End Process Runtime =========================
//============================================================
#endif

#define STACK_TYPE 6

stz_long stanza_entry (VMInit* init);

//     Command line arguments
//     ======================
stz_int input_argc;
stz_byte** input_argv;
stz_int input_argv_needs_free;

//     Main Driver
//     ===========
static void* alloc (VMInit* init, long type, long size){
  void* ptr = init->heap_top + 8;
  *(long*)(init->heap_top) = type;
  init->heap_top += 8 + size;
  return ptr;  
}

static uint64_t alloc_stack (VMInit* init){
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

  input_argc = (stz_int)argc;
  input_argv = (stz_byte **)argv;
  input_argv_needs_free = 0;
  VMInit init;

  //Allocate heap and freespace
  const int initial_heap_size = 1024 * 1024;
  const long maximum_heap_size = 4L * 1024 * 1024 * 1024;

  init.heap = (stz_byte*)stz_memory_map(initial_heap_size, maximum_heap_size);
  init.heap_limit = init.heap + initial_heap_size;
  init.heap_top = init.heap;
  init.free = (stz_byte*)stz_memory_map(initial_heap_size, maximum_heap_size);
  init.free_limit = init.free + initial_heap_size;

  //Allocate stacks
  init.current_stack = alloc_stack(&init);
  init.system_stack = alloc_stack(&init);

  //Call Stanza entry
  stanza_entry(&init);

  //Heap and freespace are disposed by OS at process termination
  return 0;
}
