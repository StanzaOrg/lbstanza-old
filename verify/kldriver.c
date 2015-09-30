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

int print_int (int x) {
  printf("x = %d\n", x);
  return 0;
}

int print_float (float x) {
  printf("x = %f\n", x);
  return 0;
}

int print_ints (int x1, int x2, int x3, int x4, int x5, int x6,
                int x7, int x8, int x9, int x10, int x11, int x12,
                int x13, int x14, int x15, int x16) {
  printf("x1 = %d\n", x1);
  printf("x2 = %d\n", x2);
  printf("x3 = %d\n", x3);
  printf("x4 = %d\n", x4);
  printf("x5 = %d\n", x5);
  printf("x6 = %d\n", x6);
  printf("x7 = %d\n", x7);
  printf("x8 = %d\n", x8);
  printf("x9 = %d\n", x9);
  printf("x10 = %d\n", x10);
  printf("x11 = %d\n", x11);
  printf("x12 = %d\n", x12);
  printf("x13 = %d\n", x13);
  printf("x14 = %d\n", x14);
  printf("x15 = %d\n", x15);
  printf("x16 = %d\n", x16);
  return 0;
}

int print_floats (float x1, float x2, float x3, float x4, float x5, float x6,
                  float x7, float x8, float x9, float x10, float x11, float x12,
                  float x13, float x14, float x15, float x16) {
  printf("x1 = %f\n", x1);
  printf("x2 = %f\n", x2);
  printf("x3 = %f\n", x3);
  printf("x4 = %f\n", x4);
  printf("x5 = %f\n", x5);
  printf("x6 = %f\n", x6);
  printf("x7 = %f\n", x7);
  printf("x8 = %f\n", x8);
  printf("x9 = %f\n", x9);
  printf("x10 = %f\n", x10);
  printf("x11 = %f\n", x11);
  printf("x12 = %f\n", x12);
  printf("x13 = %f\n", x13);
  printf("x14 = %f\n", x14);
  printf("x15 = %f\n", x15);
  printf("x16 = %f\n", x16);
  return 0;
}

int print_ints_floats (int x1, float x2, int x3, float x4, int x5, float x6,
                       int x7, float x8, int x9, float x10, int x11, float x12,
                       int x13, float x14, int x15, float x16) {
  printf("x1 = %d\n", x1);
  printf("x2 = %f\n", x2);
  printf("x3 = %d\n", x3);
  printf("x4 = %f\n", x4);
  printf("x5 = %d\n", x5);
  printf("x6 = %f\n", x6);
  printf("x7 = %d\n", x7);
  printf("x8 = %f\n", x8);
  printf("x9 = %d\n", x9);
  printf("x10 = %f\n", x10);
  printf("x11 = %d\n", x11);
  printf("x12 = %f\n", x12);
  printf("x13 = %d\n", x13);
  printf("x14 = %f\n", x14);
  printf("x15 = %d\n", x15);
  printf("x16 = %f\n", x16);
  return 0;
}

//int helper (int x0, float y0,
//            int x1, float y1,
//            int x2, float y2,
//            int x3, float y3,
//            int x4, float y4,
//            int x5, float y5,
//            int x6, float y6,
//            int x7, float y7){
//  printf("x0 = %d, y0 = %f\n", x0, y0);
//  printf("x1 = %d, y1 = %f\n", x1, y1); 
//  printf("x2 = %d, y2 = %f\n", x2, y2); 
//  printf("x3 = %d, y3 = %f\n", x3, y3); 
//  printf("x4 = %d, y4 = %f\n", x4, y4); 
//  printf("x5 = %d, y5 = %f\n", x5, y5); 
//  printf("x6 = %d, y6 = %f\n", x6, y6); 
//  printf("x7 = %d, y7 = %f\n", x7, y7); 
//  return 42;
//}
