#include<stdio.h>
#include<stdlib.h>

long helloworld (long a, long b){
  printf("Hello world!\n");
  printf("a = %ld, b = %ld\n", a, b);
  return a + b;
}

double myglobal = 0.0;

float squared (float x){
  printf("Attempt to do square with %f\n", x);
  myglobal = 2.0 * x;
  return x * x;
}

