#include<stdio.h>
#include<stdlib.h>

long myentry ();
int main () {
  long x = myentry();
  float f = *(float*)&x;
  double d = *(double*)&x;
  printf("Return Byte: %d\n", (char)x);
  printf("Return Int: %d\n", (int)x);
  printf("Return Long: %ld\n", x);
  printf("Return Float: %f\n", f);
  printf("Return Double: %lf\n", d);
  return 0;
}
