#include<stdio.h>
#include<stdlib.h>

long stanza_entry (char* stack_mem);
int main () {
  char* stack_mem = malloc(1024 * 1024);
  printf("Stack_mem = %p\n", stack_mem);
  long x = stanza_entry(stack_mem);
  float f = *(float*)&x;
  double d = *(double*)&x;
  printf("Return Int: %d\n", (int)x);
  printf("Return Long: %ld\n", x);
  printf("Return Float: %f\n", f);
  printf("Return Double: %lf\n", d);
  return 0;
}
