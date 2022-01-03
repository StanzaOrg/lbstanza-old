#include<stdio.h>

int c_callback (int i0, float f0,
                int i1, float f1,
                int i2, float f2,
                int i3, float f3,
                int i4, float f4,
                int i5, float f5,
                int i6, float f6,
                int i7, float f7,
                int i8, float f8,
                int i9, float f9) {
  printf("i0 = %d\n", i0);
  printf("i1 = %d\n", i1);
  printf("i2 = %d\n", i2);
  printf("i3 = %d\n", i3);
  printf("i4 = %d\n", i4);
  printf("i5 = %d\n", i5);
  printf("i6 = %d\n", i6);
  printf("i7 = %d\n", i7);
  printf("i8 = %d\n", i8);
  printf("i9 = %d\n", i9);
  printf("f0 = %f\n", f0);
  printf("f1 = %f\n", f1);
  printf("f2 = %f\n", f2);
  printf("f3 = %f\n", f3);
  printf("f4 = %f\n", f4);
  printf("f5 = %f\n", f5);
  printf("f6 = %f\n", f6);
  printf("f7 = %f\n", f7);
  printf("f8 = %f\n", f8);
  printf("f9 = %f\n", f9);
  int ix = i0 + i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8 + i9;
  int fx = f0 + f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + f9;
  return ix + (int)fx;
}
  
int lostanza_callback (int, float,
                       int, float,
                       int, float,
                       int, float,
                       int, float,
                       int, float,
                       int, float,
                       int, float,
                       int, float,
                       int, float);
int call_stanza_callback () {
  int i0 = 100;
  int i1 = 101;
  int i2 = 102;
  int i3 = 103;
  int i4 = 104;
  int i5 = 105;
  int i6 = 106;
  int i7 = 107;
  int i8 = 108;
  int i9 = 109;
  float f0 = 100.0f;
  float f1 = 100.1f;
  float f2 = 100.2f;
  float f3 = 100.3f;
  float f4 = 100.4f;
  float f5 = 100.5f;
  float f6 = 100.6f;
  float f7 = 100.7f;
  float f8 = 100.8f;
  float f9 = 100.9f;
  int result = lostanza_callback(i0, f0,
                                 i1, f1,
                                 i2, f2,
                                 i3, f3,
                                 i4, f4,
                                 i5, f5,
                                 i6, f6,
                                 i7, f7,
                                 i8, f8,
                                 i9, f9);
  printf("result = %d\n", result);
  return result;    
}
