#include<stdlib.h>

long stanza_entry (char* stack_mem);
long stanza_stack_size;
int main () {
  char* stack_mem = malloc(stanza_stack_size);
  stanza_entry(stack_mem);
  return 0;
}
