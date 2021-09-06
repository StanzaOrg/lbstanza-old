#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

//============================================================
//================ C Trampoline Definition ===================
//============================================================
extern void c_trampoline (void* fptr, void* argbuffer, void* retbuffer);

//============================================================
//================ Functions to be Called ====================
//============================================================

int int_myfunction_int_float (int i0, float f0,
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
  printf("f0 = %f\n", f0);
  printf("i1 = %d\n", i1);
  printf("f1 = %f\n", f1);
  printf("i2 = %d\n", i2);
  printf("f2 = %f\n", f2);
  printf("i3 = %d\n", i3);
  printf("f3 = %f\n", f3);
  printf("i4 = %d\n", i4);
  printf("f4 = %f\n", f4);
  printf("i5 = %d\n", i5);
  printf("f5 = %f\n", f5);
  printf("i6 = %d\n", i6);
  printf("f6 = %f\n", f6);
  printf("i7 = %d\n", i7);
  printf("f7 = %f\n", f7);
  printf("i8 = %d\n", i8);
  printf("f8 = %f\n", f8);
  printf("i9 = %d\n", i9);
  printf("f9 = %f\n", f9);
  return i0 + i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8 + i9;
}

int int_myfunction_int (int i0,
                        int i1,
                        int i2) {
  printf("i0 = %d\n", i0);
  printf("i1 = %d\n", i1);
  printf("i2 = %d\n", i2);
  return i0 + i1 + i2;
}

float float_myfunction_float (float f0,
                              float f1,
                              float f2) {
  printf("f0 = %f\n", f0);
  printf("f1 = %f\n", f1);
  printf("f2 = %f\n", f2);
  return f0 + f1 + f2;
}

//============================================================
//=============== Populate Argument Buffer ===================
//============================================================

uint64_t argbuffer[50];
int argindex = 0;

void fill_argbuffer (){
  argindex = 0;
}

void push_ptr (void* x){
  void** ptr = (void**)(&argbuffer[argindex]);
  *ptr = x;
  argindex++;
}

void push_long (int64_t x){
  argbuffer[argindex] = (uint64_t)x;
  argindex++;
}

void push_float (float x){
  float* ptr = (float*)(&argbuffer[argindex]);
  *ptr = x;
  argindex++;
}

void push_double (double x){
  double* ptr = (double*)(&argbuffer[argindex]);
  *ptr = x;
  argindex++;
}

void skip_arg () {
  argindex++;
}

int int_ret (){
  return (int)(argbuffer[0]);
}

float float_ret (){
  float* f = (float*)(&argbuffer[1]);
  return *f;
}

//============================================================
//=================== POSIX Calls ============================
//============================================================
#if defined(PLATFORM_OS_X) | defined(PLATFORM_LINUX)

void call1 () {
  void* f = int_myfunction_int_float;

  //i0: Int 0
  //f0: Float 0
  //i1: Int 1
  //f1: Float 1
  //i2: Int 2
  //f2: Float 2
  //i3: Int 3
  //f3: Float 3
  //i4: Int 4
  //f4: Float 4
  //i5: Int 5
  //f5: Float 5
  //i6: Stack 0
  //f6: Float 6
  //i7: Stack 1
  //f7: Float 7
  //i8: Stack 2
  //f8: Stack 3
  //i9: Stack 4
  //f9: Stack 5
  //Num stack 6
  //Num float 8
  //Num integer 6
  
  fill_argbuffer();
  //Stack arguments
  push_long(6);
  push_float(0.9f);
  push_long(109);
  push_float(0.8f);
  push_long(108);
  push_long(107);
  push_long(106);
  //Float arguments
  push_long(8);
  push_float(0.7f);
  push_float(0.6f);
  push_float(0.5f);
  push_float(0.4f);
  push_float(0.3f);
  push_float(0.2f);
  push_float(0.1f);
  push_float(0.05f);
  //Int arguments
  push_long(7);
  push_long(105);
  push_long(104);
  push_long(103);
  push_long(102);
  push_long(101);
  push_long(100);
  push_long(10);

  //Perform call
  printf("Call1()\n");
  c_trampoline(f, argbuffer, argbuffer);

  //Print result
  printf("Return = %d\n", int_ret());  
}

void call2 () {
  void* f = int_myfunction_int;

  //i0: Int 0
  //i1: Int 1
  //i2: Int 2
  //Num stack 0
  //Num float 0
  //Num integer 3
  
  fill_argbuffer();
  //Stack arguments
  push_long(0);
  //Float arguments
  push_long(0);
  //Int arguments
  push_long(4);
  push_long(102);
  push_long(101);
  push_long(100);
  push_long(0);

  //Perform call
  printf("Call2()\n");
  c_trampoline(f, argbuffer, argbuffer);

  //Print result
  printf("Return = %d\n", int_ret());    
}

void call3 () {
  void* f = printf;
  char* fmt = "IntsAndFloats\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n";
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
  float f0 = 0.1f;
  float f1 = 0.2f;
  float f2 = 0.3f;
  float f3 = 0.4f;
  float f4 = 0.5f;
  float f5 = 0.6f;
  float f6 = 0.7f;
  float f7 = 0.8f;
  float f8 = 0.9f;
  float f9 = 1.0f;

  //Argument layout
  //fmt: Int 0
  //i0:  Int 1
  //f0:  Float 0
  //i1:  Int 2
  //f1:  Float 1
  //i2:  Int 3
  //f2:  Float 2
  //i3:  Int 4
  //f3:  Float 3
  //i4:  Int 5
  //f4:  Float 4
  //i5:  Stack 0
  //f5:  Float 5
  //i6:  Stack 1
  //f6:  Float 6
  //i7:  Stack 2
  //f7:  Float 7
  //i8:  Stack 3
  //f8:  Stack 4
  //i9:  Stack 5
  //f9:  Stack 6
  //Num stack 7
  //Num float 8
  //Num integer 6
  
  fill_argbuffer();
  //Stack arguments
  push_long(7);
  push_double(f9);
  push_long(i9);
  push_double(f8);
  push_long(i8);
  push_long(i7);
  push_long(i6);
  push_long(i5);
  //Float arguments
  push_long(8);
  push_double(f7);
  push_double(f6);
  push_double(f5);
  push_double(f4);
  push_double(f3);
  push_double(f2);
  push_double(f1);
  push_double(f0);
  //Int arguments
  push_long(6 + 1);
  push_long(i4);
  push_long(i3);
  push_long(i2);
  push_long(i1);
  push_long(i0);
  push_ptr(fmt);
  push_long(10);

  printf("Call3()\n");
  c_trampoline(f, argbuffer, argbuffer);
}

void call4 () {
  void* f = float_myfunction_float;
  float f0 = 0.1f;
  float f1 = 0.2f;
  float f2 = 0.3f;
  
  //f0: Float 0
  //f1: Float 1
  //f2: Float 2
  //Num stack 0
  //Num float 3
  //Num integer 0
  
  fill_argbuffer();
  //Stack arguments
  push_long(0);
  //Float arguments
  push_long(3);
  push_float(f2);
  push_float(f1);
  push_float(f0);
  //Int arguments
  push_long(1);
  push_long(3);

  //Perform call
  printf("Call4()\n");
  c_trampoline(f, argbuffer, argbuffer);

  //Print result
  printf("Return = %f\n", float_ret());    
}

#endif

//==============================================================
//=================== Windows Calls ============================
//==============================================================
#if defined(PLATFORM_WINDOWS)

void call1 () {
  void* f = int_myfunction_int_float;
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
  float f0 = 0.05f;
  float f1 = 0.1f;
  float f2 = 0.2f;
  float f3 = 0.3f;
  float f4 = 0.4f;
  float f5 = 0.5f;
  float f6 = 0.6f;
  float f7 = 0.7f;
  float f8 = 0.8f;
  float f9 = 0.9f;

  //i0: Int 0
  //f0: Float 1, Shadow(Int 1)
  //i1: Int 2
  //f1: Float 3, Shadow(Int 3)
  //i2: Stack 0
  //f2: Stack 1
  //i3: Stack 2
  //f3: Stack 3
  //i4: Stack 4
  //f4: Stack 5
  //i5: Stack 6
  //f5: Stack 7
  //i6: Stack 8
  //f6: Stack 9
  //i7: Stack 10
  //f7: Stack 11
  //i8: Stack 12
  //f8: Stack 13
  //i9: Stack 14 
  //f9: Stack 15
  //Num stack 16
  //Num float 4
  //Num integer 4
  
  fill_argbuffer();
  //Stack arguments
  push_long(16);
  push_float(f9);
  push_long(i9);
  push_float(f8);
  push_long(i8);
  push_float(f7);
  push_long(i7);
  push_float(f6);
  push_long(i6);
  push_float(f5);
  push_long(i5);
  push_float(f4);
  push_long(i4);
  push_float(f3);
  push_long(i3);
  push_float(f2);
  push_long(i2);
  //Float arguments
  push_long(4);
  push_float(f1);
  skip_arg();
  push_float(f0);
  skip_arg();
  //Int arguments
  push_long(4 + 1);
  push_float(f1);
  push_long(i1);
  push_float(f0);
  push_long(i0);
  push_long(10);

  //Perform call
  printf("Call1()\n");
  c_trampoline(f, argbuffer, argbuffer);

  //Print result
  printf("Return = %d\n", int_ret());  
}

void call2 () {
  void* f = int_myfunction_int;
  int i0 = 100;
  int i1 = 101;
  int i2 = 102;

  //i0: Int 0
  //i1: Int 1
  //i2: Int 2
  //Num stack 0
  //Num float 0
  //Num integer 3
  
  fill_argbuffer();
  //Stack arguments
  push_long(0);
  //Float arguments
  push_long(0);
  //Int arguments
  push_long(3 + 1);
  push_long(i2);
  push_long(i1);
  push_long(i0);
  push_long(0);

  //Perform call
  printf("Call2()\n");
  c_trampoline(f, argbuffer, argbuffer);

  //Print result
  printf("Return = %d\n", int_ret());    
}

void call3 () {
  void* f = printf;
  char* fmt = "IntsAndFloats\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n(%d,%f)\n";
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
  float f0 = 0.1f;
  float f1 = 0.2f;
  float f2 = 0.3f;
  float f3 = 0.4f;
  float f4 = 0.5f;
  float f5 = 0.6f;
  float f6 = 0.7f;
  float f7 = 0.8f;
  float f8 = 0.9f;
  float f9 = 1.0f;

  //Argument layout
  //fmt: Int 0
  //i0: Int 1
  //f0: Float 2, Shadow(Int 2)
  //i1: Int 3
  //f1: Stack 0
  //i2: Stack 1
  //f2: Stack 2
  //i3: Stack 3
  //f3: Stack 4
  //i4: Stack 5
  //f4: Stack 6
  //i5: Stack 7
  //f5: Stack 8
  //i6: Stack 9
  //f6: Stack 10
  //i7: Stack 11
  //f7: Stack 12
  //i8: Stack 13
  //f8: Stack 14
  //i9: Stack 15
  //f9: Stack 16
  //Num stack 17
  //Num float 3
  //Num integer 4
  
  fill_argbuffer();
  //Stack arguments
  push_long(17);
  push_double(f9);
  push_long(i9);
  push_double(f8);
  push_long(i8);
  push_double(f7);
  push_long(i7);
  push_double(f6);
  push_long(i6);
  push_double(f5);
  push_long(i5);
  push_double(f4);
  push_long(i4);
  push_double(f3);
  push_long(i3);
  push_double(f2);
  push_long(i2);
  push_double(f1); 
  //Float arguments
  push_long(3);
  push_double(f0);
  skip_arg();
  skip_arg();
  //Int arguments
  push_long(4 + 1);
  push_long(i3);
  push_double(f0);
  push_long(i0);
  push_ptr(fmt);
  push_long(10);

  printf("Call3()\n");
  c_trampoline(f, argbuffer, argbuffer);
}

void call4 () {
  void* f = float_myfunction_float;
  float f0 = 0.1f;
  float f1 = 0.2f;
  float f2 = 0.3f;
  
  //f0: Float 0, Shadow(Int 0)
  //f1: Float 1, Shadow(Int 1)
  //f2: Float 2, Shadow(Int 2)
  //Num stack 0
  //Num float 3
  //Num integer 3
  
  fill_argbuffer();
  //Stack arguments
  push_long(0);
  //Float arguments
  push_long(3);
  push_float(f2);
  push_float(f1);
  push_float(f0);
  //Int arguments
  push_long(3 + 1);
  push_float(f2);
  push_float(f1);
  push_float(f0);
  push_long(3);

  //Perform call
  printf("Call4()\n");
  c_trampoline(f, argbuffer, argbuffer);

  //Print result
  printf("Return = %f\n", float_ret());    
}

#endif

//==============================================================
//==================== Main Launcher ===========================
//==============================================================

int main (int argc, char** argvs){
  call1();
  call2();
  call3();
  call4();
  return 0;
}
