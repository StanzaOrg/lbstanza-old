#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>

extern c_trampoline (uint64_t f, uint64_t* args, uint64_t* ret);

float myplus (float x, float y){
  printf("Adding %f to %f\n", x, y);
  return x + y;
}

void main () {
  uint64_t* args = malloc(30 * sizeof(uint64_t));
  uint64_t* ret = malloc(2 * sizeof(uint64_t));
  uint64_t* ptr = args;
  //Stack arguments
  *ptr = 0; ptr++;
  //Floating point arguments
  *ptr = 2; ptr++;
  *(float*)ptr = 42.0; ptr++;
  *(float*)ptr = 7.0; ptr++;
  //Integer arguments
  *ptr = 1; ptr++;
  *ptr = 2; ptr++;
  
  for(int i=0; i<10; i++){
    uint64_t f = (uint64_t)&myplus;
    c_trampoline(f, args, ret);
    printf("Result Rax = %d, Xmm0 = %f\n", *(int*)ret, *(float*)(ret + 1));
    printf("Hello world %d %d\n", i, i * 10);
  }
}
