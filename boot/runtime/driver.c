#include<stdlib.h>
#include<stdio.h>

long stanza_entry (char* stack_mem);
long stanza_stack_size;

int main () {
  char* stack_mem = malloc(stanza_stack_size);
  stanza_entry(stack_mem);
  return 0;
}

FILE* get_stdout () {return stdout;}
FILE* get_stderr () {return stderr;}
FILE* get_stdin () {return stdin;}
