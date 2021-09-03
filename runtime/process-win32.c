#include <windows.h>
#include <namedpipeapi.h>
#include <processthreadsapi.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdbool.h>

#include "common.h"
#include "process.h"
#include "types.h"

typedef enum {
  FT_READ,
  FT_WRITE
} FileType;

static FILE* file_from_handle(HANDLE handle, FileType type) {
  int fd;
  int osflags;
  char* mode;
  FILE* file;

  switch (type) {
    case FT_READ:  osflags = _O_RDONLY | _O_BINARY; break;
    case FT_WRITE: osflags = _O_WRONLY | _O_BINARY; break;
  }

  switch (type) {
    case FT_READ:  mode = "rb"; break;
    case FT_WRITE: mode = "wb"; break;
  }

  fd = _open_osfhandle((intptr_t)handle, osflags);
  if (fd == -1) return NULL;

  file = _fdopen(fd, mode);
  if (file == NULL) return NULL;

  setvbuf(file, NULL, _IONBF, 0);

  return file;
}

void retrieve_process_state (stz_long pid, ProcessState* s, stz_int wait_for_termination) {
  ProcessState state;
  HANDLE process;
  DWORD exit_code;

  state = (ProcessState){PROCESS_RUNNING, 0};

  process = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, (DWORD)pid);
  if (process == NULL) {
    goto END;
  }

  if (wait_for_termination == 1) {
    if (WaitForSingleObject(process, INFINITE) == WAIT_FAILED) {
      goto END;
    }
  }

  if (!GetExitCodeProcess(process, &exit_code)) {
    goto END;
  }

  if (exit_code != STILL_ACTIVE) {
    state = (ProcessState){PROCESS_DONE, (stz_int)exit_code};
  }

END:
  *s = state;
}

typedef enum {
  PIPE_IN,
  PIPE_OUT
} PipeType;

static void create_pipe(PHANDLE read, PHANDLE write, PipeType type) {
  SECURITY_ATTRIBUTES security_attrs;
  PHANDLE our_end;

  ZeroMemory(&security_attrs, sizeof(SECURITY_ATTRIBUTES));
  security_attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
  security_attrs.lpSecurityDescriptor = NULL;
  security_attrs.bInheritHandle = TRUE;

  switch (type) {
    case PIPE_IN:  our_end = read;  break;
    case PIPE_OUT: our_end = write; break;
  }

  CreatePipe(read, write, &security_attrs, 0);
  SetHandleInformation(*our_end, HANDLE_FLAG_INHERIT, 0);
}

static HANDLE duplicate_standard_handle(int handle) {
  HANDLE ret;

  if (!DuplicateHandle(
        /* hSourceProcessHandle */ GetCurrentProcess(),
        /* hSourceHandle        */ GetStdHandle(handle),
        /* hTargetProcessHandle */ GetCurrentProcess(),
        /* lpTargetHandle       */ &ret,
        /* dwDesiredAccess      */ 0,
        /* bInheritHandle       */ TRUE,
        /* dwOptions            */ DUPLICATE_SAME_ACCESS)) {
    return NULL;
  }

  return ret;
}

static void setup_file_handles(
    stz_int input, stz_int output, stz_int error,
    PHANDLE process_stdin_read, PHANDLE process_stdin_write,
    PHANDLE process_stdout_read, PHANDLE process_stdout_write,
    PHANDLE process_stderr_read, PHANDLE process_stderr_write) {

  HANDLE stdin_read, stdin_write,
         stdout_read, stdout_write,
         stderr_read, stderr_write;

  // Initialize all our handles to NULL (just in case)
  *process_stdin_read   = NULL;
  *process_stdin_write  = NULL;
  *process_stdout_read  = NULL;
  *process_stdout_write = NULL;
  *process_stderr_read  = NULL;
  *process_stderr_write = NULL;

  // Compute which pipes we want to open
  int pipe_sources[NUM_STREAM_SPECS] = { -1 };
  pipe_sources[input]  = 0;
  pipe_sources[output] = 1;
  pipe_sources[error]  = 2;

  // Open the pipes we requested
  if (pipe_sources[PROCESS_IN] >= 0) {
    create_pipe(&stdin_read, &stdin_write, PIPE_OUT);
  }
  if (pipe_sources[PROCESS_OUT] >= 0) {
    create_pipe(&stdout_read, &stdout_write, PIPE_IN);
  }
  if (pipe_sources[PROCESS_ERR] >= 0) {
    create_pipe(&stderr_read, &stderr_write, PIPE_IN);
  }

  // Input can either be redirected to an IN pipe or re-use parent's STDIN
  if (input == PROCESS_IN) {
    *process_stdin_read  = stdin_read;
    *process_stdin_write = stdin_write;
  }
  else {
    *process_stdin_read  = duplicate_standard_handle(STD_INPUT_HANDLE);
    *process_stdin_write = NULL;
  }

  // Output can be redirected to an OUT or ERR pipe or re-use parent's STDOUT
  if (output == PROCESS_OUT) {
    *process_stdout_read  = stdout_read;
    *process_stdout_write = stdout_write;
  }
  else if (output == PROCESS_ERR) {
    *process_stderr_read  = stdout_read;
    *process_stderr_write = stdout_write;
  }
  else {
    *process_stdout_read  = NULL;
    *process_stdout_write = duplicate_standard_handle(STD_OUTPUT_HANDLE);
  }

  // Error can be redirected to an OUT or ERR pipe or re-use parent's STDERR
  if (error == PROCESS_OUT) {
    *process_stderr_read  = stdout_read;
    *process_stderr_write = stdout_write;
  }
  else if (error == PROCESS_ERR) {
    *process_stderr_read  = stderr_read;
    *process_stderr_write = stderr_write;
  }
  else {
    *process_stderr_read  = NULL;
    *process_stderr_write = duplicate_standard_handle(STD_ERROR_HANDLE);
  }
}

stz_int launch_process(stz_byte* command_line, stz_int input, stz_int output, stz_int error, Process* process) {
  PROCESS_INFORMATION proc_info;
  STARTUPINFO start_info;
  HANDLE stdin_read, stdin_write,
         stdout_read, stdout_write,
         stderr_read, stderr_write;
  BOOL success;

  // Set up our STDIN, STDOUT, and STDERR
  setup_file_handles(input, output, error,
      &stdin_read, &stdin_write,
      &stdout_read, &stdout_write,
      &stderr_read, &stderr_write
  );

  // Now that we have our handles, set up STARTUPINFO so that the child process will use them
  ZeroMemory(&start_info, sizeof(STARTUPINFO));
  start_info.cb = sizeof(STARTUPINFO);
  start_info.hStdInput  = stdin_read;
  start_info.hStdOutput = stdout_write;
  start_info.hStdError  = stderr_write;
  start_info.dwFlags |= STARTF_USESTDHANDLES;

  // Zero out our process information in ancipication of CreateProcess
  // populating it, and then launch our process.
  ZeroMemory(&proc_info, sizeof(PROCESS_INFORMATION));
  success = CreateProcess(
      /* lpApplicationName    */ NULL,
      /* lpCommandLine        */ (LPSTR)command_line,
      /* lpProcessAttributes  */ NULL,
      /* lpThreadAttributes   */ NULL,
      /* bInheritHandles      */ TRUE,
      /* dwCreationFlags      */ 0,
      /* lpEnvironment        */ NULL,
      /* lpCurrentDirectory   */ NULL,
      /* lpStartupInfo        */ &start_info,
      /* lpProcessInformation */ &proc_info);

  if (success) {
    // Populate process with the relevant info
    process->pid = (stz_long)proc_info.dwProcessId;
    process->pipeid = -1; // -1 signals we didn't create named pipes for this process
    process->in  = file_from_handle(stdin_write, FT_WRITE);
    process->out = file_from_handle(stdout_read, FT_READ);
    process->err = file_from_handle(stderr_read, FT_READ);

    // Close the handles we passed into the child process
    if (stdin_read   != NULL) CloseHandle(stdin_read);
    if (stdout_write != NULL) CloseHandle(stdout_write);
    if (stderr_write != NULL) CloseHandle(stderr_write);

    // Close these left-over handles to the process
    CloseHandle(proc_info.hThread);
    CloseHandle(proc_info.hProcess);
  }

  return success ? 0 : -1;
}
