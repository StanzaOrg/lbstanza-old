#include<stdlib.h>
#include<stdio.h>

long stanza_entry (char* stack_mem);
extern long stanza_stack_size;

//Command line arguments
long input_argc;
char** input_argv;

int main (int argc, char* argv[]) {
  input_argc = argc;
  input_argv = argv;
  char* stack_mem = malloc(stanza_stack_size);
  stanza_entry(stack_mem);
  return 0;
}

FILE* get_stdout () {return stdout;}
FILE* get_stderr () {return stderr;}
FILE* get_stdin () {return stdin;}
int get_eof () {return EOF;}
