#ifndef RUNTIME_PROCESS_H
#define RUNTIME_PROCESS_H
#include <stdio.h>
#include <stanza/types.h>

//Represents the Process, and the channels for
//communicating with it.
//- pid: The id of the process. 
//- handle: The Windows handle to the process. Not used by other platforms.
//- pipeid: A unique integer used to generate the names of the named pipes for communication.
//          Will be set to -1 for Windows to indicate that no named pipes are created.
//- in: The standard input stream of the Process.
//- out: The standard output stream of the Process.
//- err: The standard error stream of the Process.
typedef struct {
  stz_long pid;
  void* handle;
  stz_int pipeid;
  FILE* in;
  FILE* out;
  FILE* err;
} Process;

//Represents the state of the process.
//- state: PROCESS_RUNNING | PROCESS_DONE | PROCESS_TERMINATED | PROCESS_STOPPED
//- code: If state is PROCESS_RUNNING, then code is 0, otherwise, state is the
//  exit code of the process.
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
