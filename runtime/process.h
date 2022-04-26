#ifndef RUNTIME_PROCESS_H
#define RUNTIME_PROCESS_H

#include <stdio.h>

#include <stanza/types.h>

typedef struct {
  stz_long pid;
  void* handle;
  stz_int pipeid;
  FILE* in;
  FILE* out;
  FILE* err;
} Process;

typedef struct {
  stz_int state;
  stz_int code;
} ProcessState;

#define PROCESS_RUNNING 0
#define PROCESS_DONE 1
#define PROCESS_TERMINATED 2
#define PROCESS_STOPPED 3

#define STANDARD_IN 0
#define STANDARD_OUT 1
#define PROCESS_IN 2
#define PROCESS_OUT 3
#define STANDARD_ERR 4
#define PROCESS_ERR 5
#define NUM_STREAM_SPECS 6

#endif
