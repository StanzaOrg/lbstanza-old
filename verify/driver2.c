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

int helper (int x0, float y0,
            int x1, float y1,
            int x2, float y2,
            int x3, float y3,
            int x4, float y4,
            int x5, float y5,
            int x6, float y6,
            int x7, float y7){
  printf("x0 = %d, y0 = %f\n", x0, y0);
  printf("x1 = %d, y1 = %f\n", x1, y1); 
  printf("x2 = %d, y2 = %f\n", x2, y2); 
  printf("x3 = %d, y3 = %f\n", x3, y3); 
  printf("x4 = %d, y4 = %f\n", x4, y4); 
  printf("x5 = %d, y5 = %f\n", x5, y5); 
  printf("x6 = %d, y6 = %f\n", x6, y6); 
  printf("x7 = %d, y7 = %f\n", x7, y7); 
  return 42;
}
