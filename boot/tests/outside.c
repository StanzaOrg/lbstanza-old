#include<stdlib.h>
#include<stdio.h>

long silly_inc (long x);
long silly_add (long x, long y) {
  printf("silly_add(%ld, %ld)\n", x, y);
  while(y > 0){
    x = silly_inc(x);
    y = y - 1;
  }
  return x;
}
