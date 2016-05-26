#ifdef PLATFORM_WINDOWS
  #include<Windows.h>
#endif
#include<stdlib.h>
#include<stdio.h>
#include<sys/time.h>
#include<errno.h>

//     Stanza Defined Entities
//     =======================
long stanza_entry (char* stack_mem);
extern long stanza_stack_size;

//     Command line arguments
//     ======================
long input_argc;
char** input_argv;

//     Main Driver
//     ===========
int main (int argc, char* argv[]) {  
  input_argc = argc;
  input_argv = argv;
  char* stack_mem = (char*)malloc(stanza_stack_size);
  stanza_entry(stack_mem);
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
long current_time_us (void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}

long current_time_ms (void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
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
  char* resolve_path (char* filename){
    return realpath(filename, 0);
  }
#endif
