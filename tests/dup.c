#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<errno.h>
#include <fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>

//============================================================
//=================== Structures =============================
//============================================================

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
  char* file;
  char** argvs;
} EvalArg;

#define PROCESS_RUNNING 0
#define PROCESS_DONE 1
#define PROCESS_TERMINATED 2
#define PROCESS_STOPPED 3

//============================================================
//==================== Utilities =============================
//============================================================

void exit_with_error (){
  fprintf(stderr, "%s\n", strerror(errno));
  exit(-1);
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

//============================================================
//==================== Process Creation ======================
//============================================================

void make_pipes (char* name){
  int len = strlen(name);
  char* buffer = (char*)malloc(len + 10);
  sprintf(buffer, "%s_in", name);
  mkfifo(buffer, S_IRUSR|S_IWUSR);
  sprintf(buffer, "%s_out", name);
  mkfifo(buffer, S_IRUSR|S_IWUSR);
  sprintf(buffer, "%s_err", name);
  mkfifo(buffer, S_IRUSR|S_IWUSR);
}

long fork_child (char* pipe, int (*f)(void*), void* arg){
  //Fork
  long pid = (long)fork();
  if(pid < 0){
    fprintf(stderr, "Could not fork.\n");
    fprintf(stderr, "%s\n", strerror(errno));
    exit(-1);
  }

  //If parent
  if(pid > 0){
    return pid;
  }else{
    dup2(open_pipe(pipe, "_in", O_RDONLY), 0);
    dup2(open_pipe(pipe, "_out", O_WRONLY), 1);
    dup2(open_pipe(pipe, "_err", O_WRONLY), 2);
    exit(f(arg));
  }
}

FILE* try_fdopen (int fd, char* mode){
  FILE* ret = fdopen(fd, mode);
  if(ret == NULL){
    fprintf(stderr, "%s\n", strerror(errno));
    exit(-1);
  }
  return ret;
}

Process make_process (long pid, char* pipe){
  Process p = {
    pid,
    try_fdopen(open_pipe(pipe, "_in", O_WRONLY), "w"),
    try_fdopen(open_pipe(pipe, "_out", O_RDONLY), "r"),
    try_fdopen(open_pipe(pipe, "_err", O_RDONLY), "r")};
  return p;
}

//Process make_process (char* in, char* out, char* err, int (*f)(void*), void* arg) {
//  //File descriptors
//  int READ = 0;
//  int WRITE = 1;
//  int ERR = 2;
//  int afd[2];
//  int bfd[2];
//  int efd[2];
//  //Parent writes to A, reads from B
//  //Child reads from A, writes to B
//
//  //Make file descriptors
//  if(pipe(afd) < 0 || pipe(bfd) < 0 || pipe(efd) < 0){
//    fprintf(stderr, "Could not create pipes.\n");
//    exit(-1);
//  }
//
//  //Fork
//  long pid = (long)fork();
//  if(pid < 0){
//    fprintf(stderr, "Could not fork.\n");
//    exit(-1);
//  }
//
//  //If parent
//  if(pid > 0){
//    close(afd[READ]);
//    close(bfd[WRITE]);
//    close(efd[WRITE]);
//    Process p = {pid, afd[WRITE], bfd[READ], efd[READ]};
//    return p;
//  }else{
//    close(afd[WRITE]);
//    close(bfd[READ]);
//    close(efd[READ]);
//    dup2(afd[READ], 0);
//    dup2(bfd[WRITE], 1);
//    dup2(efd[WRITE], 2);
//    exit(f(arg));
//  }
//}

//============================================================
//======================= Process Daemon =====================
//============================================================

// ===== Serialization =====
void write_int (FILE* f, int x){
  fwrite(&x, sizeof(int), 1, f);
}
void write_long (FILE* f, long x){
  fwrite(&x, sizeof(long), 1, f);
}
void write_string (FILE* f, char* s){
  int n = strlen(s);
  write_int(f, n);
  fwrite(s, 1, n, f);
}
int count_non_null (void** xs){
  int n=0;
  while(xs[n] != NULL)
    n++;
  return n;
}
void write_strings (FILE* f, char** s){
  int n = count_non_null((void**)s);
  write_int(f, n);
  for(int i=0; i<n; i++)
    write_string(f, s[i]);
}
void write_earg (FILE* f, EvalArg* earg){
  write_string(f, earg->pipe);
  write_string(f, earg->file);
  write_strings(f, earg->argvs);
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
  char* s = (char*)malloc(n + 1);
  bread(s, 1, n, f);
  s[n] = '\0';
  return s;
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
  earg->file = read_string(f);
  earg->argvs = read_strings(f);
  return earg;
}

//===== Free =====
void free_earg (EvalArg* arg){
  free(arg->pipe);
  free(arg->file);
  for(int i=0; arg->argvs[i] != NULL; i++)
    free(arg->argvs[i]);
  free(arg->argvs);
}

////===== Process Daemon =====
//int eval_process (void* arg0){
//  EvalArg* arg = arg0;
//  return execvp(arg->file, arg->argvs);
//}
//
//int process_daemon (void* dummy) {
//  while(1){
//    EvalArg* earg = read_earg(0);
//    printf("read arg = %s\n", earg->file); fflush(stdout);
//    //long pid = fork_child(earg->pipe, &eval_process, earg);
//    //write(1, &pid, sizeof(long));
//    //Process p = {42L, 42, 42, 42};
//    // Process p = make_process(&eval_process, earg);
//    // write_process(1, &p);
//  }
//}
//
//Process launcher_process;
//void initialize_launcher_process (){
//  make_pipes("pipe0");
//  long pid = fork_child("pipe0", &process_daemon, NULL);
//  printf("Id of launcher process = %ld\n", pid);
//  launcher_process = make_process(pid, "pipe0");
//}
//
//Process launch_process (EvalArg* arg){
//  char* pipe = "pipe1";
//  arg->pipe = pipe;
//  make_pipes(pipe);
//  write_earg(launcher_process.in, arg);
//  fflush(launcher_process.in);
//  long pid = read_long(launcher_process.out);
//  return make_process(pid, pipe);
//}

//============================================================
//==================== Process Queries =======================
//============================================================

ProcessState process_state (Process p){
  int status;
  int ret = waitpid((pid_t)p.pid, &status, WNOHANG);
  if(ret == 0){
    ProcessState s = {PROCESS_RUNNING, 0};
    return s;
  }
  else if(WIFEXITED(status)){
    ProcessState s = {PROCESS_DONE, WEXITSTATUS(status)};
    return s;
  }
  else if(WIFSIGNALED(status)){
    ProcessState s = {PROCESS_TERMINATED, WTERMSIG(status)};
    return s;
  }
  else if(WIFSTOPPED(status)){
    ProcessState s = {PROCESS_STOPPED, WSTOPSIG(status)};
    return s;
  }
  else{
    ProcessState s = {PROCESS_RUNNING, 0};
    return s;
  }
}

//============================================================
//==================== Scratch ===============================
//============================================================

//int read_all (Process p){
//  while(1){
//    printf("Attempt a read\n");
//    char buffer[100];
//    char* ret = fgets(buffer, 100, p.out);
//    if(ret == NULL){
//      ProcessState s = process_state(p);
//      if(s.state != PROCESS_RUNNING){
//        printf("Process is done with exit code %d.\n", s.code);
//        return 0;
//      }
//    }else{
//      ret[strlen(ret) - 1] = '\0';
//      printf("Read line: '%s'\n", buffer);
//    }
//  }
//}

//int main3 (void){
//  Process p = make_process(&child_process, NULL);
//  return read_all(p);
//}
//
//int main2 (void){
//  char* argvs[5];
//  argvs[0] = "ls";
//  argvs[1] = NULL;
//  EvalArg eval_arg = {argvs[0], argvs};
//  Process p = make_process(&eval_process, &eval_arg);
//
//  return read_all(p);
//}

//int main (void){  
//  char* argvs[5];
//  argvs[0] = "./child";
//  argvs[1] = NULL;
//  EvalArg eval_arg = {argvs[0], argvs};
//  Process p = make_process(&eval_process, &eval_arg);
//  while(1){
//    //Ask number
//    int i = rand() % 7;
//    printf("Asking %d\n", i);
//    fprintf(p.in, "%d\n\n", i);
//    fflush(p.in);
//
//    //Get response
//    char buffer[100];
//    char* ret = fgets(buffer, 100, p.out);
//    if(ret == NULL){
//      printf("No response\n");
//      ProcessState s = process_state(p);
//      if(s.state != PROCESS_RUNNING){
//        printf("Process is done with exit code %d.\n", s.code);
//        return 0;
//      }
//    }else{
//      ret[strlen(ret) - 1] = '\0';
//      printf("Response: '%s'\n", ret);
//    }
//  }
//}
//
//int main (void){
//  //Step one
//  initialize_launcher_process();
//
//  //Create eval argument
//  char* argvs[5];
//  argvs[0] = "./child";
//  argvs[1] = NULL;
//  EvalArg eval_arg = {"pipe", argvs[0], argvs};
//  
//  //Write the eval argument
//  write_earg(launcher_process.in, &eval_arg);
//  fflush(launcher_process.in);
//
//  char buffer[100];
//  fgets(buffer, 100, launcher_process.out);
//  printf("received %s\n", buffer);
//  
//  //Process p = launch_process(&eval_arg);
//  //printf("p.id = %ld\n", p.pid);
//  //printf("p.in = %d\n", p.in);
//  return 0;
//}


//============================================================
//============================================================
//============================================================
//
//int launcher_main (int lin_fd, int lout_fd){
//  //Open streams
//  FILE* lin = fdopen(lin_fd, "r");
//  if(lin == NULL) exit_with_error();
//  FILE* lout = fdopen(lout_fd, "w");
//  if(lout == NULL) exit_with_error();
//  
//  while(1){    
//    printf("Read earg\n");
//    EvalArg* earg = read_earg(lin);    
//    printf("file = %s\n", earg->file);
//    write_long(lout, 42L); fflush(lout);
//  }
//}
//
//FILE* launcher_in;
//FILE* launcher_out;
//void initialize_launcher_process (){
//  //Create pipes
//  int READ = 0;
//  int WRITE = 1;
//  int lin[2];
//  int lout[2];
//  if(pipe(lin) < 0) exit_with_error();
//  if(pipe(lout) < 0) exit_with_error();
//
//  //Fork
//  long pid = (long)fork();
//  if(pid < 0) exit_with_error();
//
//  if(pid > 0){
//    //parent
//    close(lin[READ]);
//    close(lout[WRITE]);
//    launcher_in = fdopen(lin[WRITE], "w");
//    if(launcher_in == NULL) exit_with_error();
//    launcher_out = fdopen(lout[READ], "r");
//    if(launcher_out == NULL) exit_with_error();
//  }else{
//    //child
//    close(lin[WRITE]);
//    close(lout[READ]);
//    exit(launcher_main(lin[READ], lout[WRITE]));
//  }
//}
//
//void launch_eval (char* file, char** argvs){
//  //Create eval argument
//  EvalArg arg;
//  arg.pipe = "pipe";
//  arg.file = file;
//  arg.argvs = argvs;
//  //Write to daemon process
//  printf("Write earg\n");
//  write_earg(launcher_in, &arg);
//  fflush(launcher_in);
//  //Read back process id
//  printf("Read back process\n");
//  long pid = read_long(launcher_out);
//  //Print out process id
//  printf("Launched eval process %ld\n", pid);
//
//  printf("Finish Writing\n");
//  write_earg(launcher_in, &arg);
//  fflush(launcher_in);
//}
//
//int main (void){
//  initialize_launcher_process();
//  
//  char* argvs[] = {"./child", NULL};
//  launch_eval("./child", argvs);
//}

//============================================================
//============================================================
//============================================================

//Daemon process for launching child processes
void launcher_main (FILE* lin, FILE* lout){
  while(1){
    //Read in evaluation arguments
    EvalArg* earg = read_earg(lin);
    if(feof(lin)) exit(0);

    //Fork a new child
    long pid = (long)fork();
    if(pid < 0) exit_with_error();

    if(pid > 0){
      //Free the evaluation arg
      free_earg(earg);
      //Return new process id
      write_long(lout, pid);
      fflush(lout);
    }
    else{
      //Open named pipes
      dup2(open_pipe(earg->pipe, "_in", O_RDONLY), 0);
      dup2(open_pipe(earg->pipe, "_out", O_WRONLY), 1);
      dup2(open_pipe(earg->pipe, "_err", O_WRONLY), 2);
      //Launch child process      
      execvp(earg->file, earg->argvs);
      exit_with_error();
    }
  }
}

long launcher_pid = -1;
FILE* launcher_in = NULL;
FILE* launcher_out = NULL;
void initialize_launcher_process (){
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

void print_all (FILE* f){
  char buffer[1000];
  while(1){
    char* r = fgets(buffer, 1000, f);
    if(ferror(f)) exit_with_error();
    if(r == NULL) break;
    printf("%s", r);
    if(feof(f)) break;
  }
}

Process* launch_process (char* file, char** argvs){
  //Initialize launcher if necessary
  if(launcher_pid < 0)
    initialize_launcher_process();
  
  //Figure out unique pipe name
  char pipe_name[80];
  sprintf(pipe_name, "/tmp/stanza_exec_pipe%ld", (long)getpid());
  
  //Create pipes to child
  make_pipe(pipe_name, "_in");
  make_pipe(pipe_name, "_out");
  make_pipe(pipe_name, "_err");
  
  //Write in evaluation arguments
  EvalArg earg = {pipe_name, file, argvs};
  write_earg(launcher_in, &earg);
  fflush(launcher_in);
  
  //Read back process id
  long pid = read_long(launcher_out);
  
  //Open pipes to child
  int in = open_pipe(pipe_name, "_in", O_WRONLY);
  int out = open_pipe(pipe_name, "_out", O_RDONLY);
  int err = open_pipe(pipe_name, "_err", O_RDONLY);
  FILE* fin = fdopen(in, "w");
  if(fin == NULL) exit_with_error();
  FILE* fout = fdopen(out, "r");
  if(fout == NULL) exit_with_error();
  FILE* ferr = fdopen(err, "r");
  if(ferr == NULL) exit_with_error();
  
  //Return process structure
  Process* p = (Process*)malloc(sizeof(Process));
  p->pid = pid;
  p->in = fin;
  p->out = fout;
  p->err = ferr;
  return p;
}

int main (void){
  char* argvs[] = {"ls", NULL};
  Process* p = launch_process("ls", argvs);
  print_all(p->out);
}
