#include <windows.h>
#include <namedpipeapi.h>
#include <processthreadsapi.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdbool.h>

#include <stanza/types.h>

#include "process.h"

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

// Polls a running `process` returning the `ProcessState`, optionally blocking until it has terminated.
int retrieve_process_state (const Process* process, ProcessState* s, stz_int wait_for_termination) {
  // If the wait_for_termination argument has been passed, we 
  // call WaitForSingleObject until the process exits.
  if (wait_for_termination) {
    // if WaitForSingleObject fails, return an error. The caller must retrieve the
    // platform error message and raise an exception.
    if (WaitForSingleObject(process->handle, INFINITE) == WAIT_FAILED) {
      return -1;
    }
  }

  // Attempt to get the exit code from the handle. If this fails, return an error. The
  // caller must retrieve the platform error message and return an exception.
  DWORD exit_code;
  if (!GetExitCodeProcess(process->handle, &exit_code)) {
    return -1;
  }

  // If exit code is STILL_ACTIVE, then process might still be running.
  if (exit_code == STILL_ACTIVE){
    //Now we have to check whether the process is actually still running,
    //or whether it finished, but happened to finish with STILL_ACTIVE as
    //its exit code. (This is bad manners, but programs are still allowed to do it.)

    //Call WaitForSingleObject with timeout = 0.
    DWORD wait_result = WaitForSingleObject(process->handle, 0);
    
    //If wait timed out, then the process is indeed still running.
    if(wait_result == WAIT_TIMEOUT){
      s->state = PROCESS_RUNNING;
      s->code = 0;
      return 0;
    }
    //Otherwise, the process happened to finish. And STILL_ACTIVE is
    //its exit code.
    else{
      s->state = PROCESS_DONE;
      s->code = exit_code;
      return 0;
    }
  }
  // The exit code is not STILL_ACTIVE, so the process is DONE.
  else {
    s->state = PROCESS_DONE;
    s->code = exit_code;
    return 0;
  }
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

static HANDLE duplicate_handle(HANDLE handle) {
  HANDLE ret;

  if (!DuplicateHandle(
        /* hSourceProcessHandle */ GetCurrentProcess(),
        /* hSourceHandle        */ handle,
        /* hTargetProcessHandle */ GetCurrentProcess(),
        /* lpTargetHandle       */ &ret,
        /* dwDesiredAccess      */ 0,
        /* bInheritHandle       */ TRUE,
        /* dwOptions            */ DUPLICATE_SAME_ACCESS)) {
    return NULL;
  }

  return ret;
}

// Set up the file handles for the process we are about to create, in order to
// redirect its STDIN/STDOUT/STDERR to the corresponding standard stream or
// pipe. The following redirections are supported:
//   STDIN -> STANDARD-IN,
//   STDIN -> PROCESS-IN,
//   STDOUT -> STANDARD-OUT,
//   STDOUT -> PROCESS-OUT,
//   STDOUT -> PROCESS-ERR,
//   STDERR -> STANDARD-ERR,
//   STDERR -> PROCESS-OUT,
//   STDERR -> PROCESS-ERR,
//
// NOTE: We are careful to duplicate all handles that are passed into the child
// process so that we do not inadvertently assign the same handle to two
// different streams (for example when both STDOUT/STDERR are redirected to
// PROCESS-OUT or to PROCESS-ERR). This is not strictly invalid, but would
// cause an error when closing these handles after launching the process.
// - input, output, error: Enums that indicate where the process communication streams
//   should be directed. Will be: STANDARD_IN | STANDARD_OUT | PROCESS_IN | PROCESS_OUT |
//   STANDARD_ERR | PROCESS_ERR.
static void setup_file_handles(
    stz_int input, stz_int output, stz_int error,
    PHANDLE process_stdin_read, PHANDLE process_stdin_write,
    PHANDLE process_stdout_read, PHANDLE process_stdout_write,
    PHANDLE process_stderr_read, PHANDLE process_stderr_write) {

  HANDLE stdin_read  = NULL, stdin_write = NULL,
         stdout_read = NULL, stdout_write = NULL,
         stderr_read = NULL, stderr_write = NULL;

  // Initialize all our handles to NULL (just in case)
  *process_stdin_read   = NULL; *process_stdin_write  = NULL;
  *process_stdout_read  = NULL; *process_stdout_write = NULL;
  *process_stderr_read  = NULL; *process_stderr_write = NULL;

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
    *process_stdin_read  = duplicate_handle(stdin_read);
    *process_stdin_write = stdin_write;
  }
  else {
    *process_stdin_read  = duplicate_handle(GetStdHandle(STD_INPUT_HANDLE));
    *process_stdin_write = NULL;
  }

  // Output can be redirected to an OUT or ERR pipe or re-use parent's STDOUT
  if (output == PROCESS_OUT) {
    *process_stdout_read  = stdout_read;
    *process_stdout_write = duplicate_handle(stdout_write);
  }
  else if (output == PROCESS_ERR) {
    *process_stderr_read  = stdout_read;
    *process_stderr_write = duplicate_handle(stdout_write);
  }
  else {
    *process_stdout_read  = NULL;
    *process_stdout_write = duplicate_handle(GetStdHandle(STD_OUTPUT_HANDLE));
  }

  // Error can be redirected to an OUT or ERR pipe or re-use parent's STDERR
  if (error == PROCESS_OUT) {
    *process_stderr_read  = stdout_read;
    *process_stderr_write = duplicate_handle(stdout_write);
  }
  else if (error == PROCESS_ERR) {
    *process_stderr_read  = stderr_read;
    *process_stderr_write = duplicate_handle(stderr_write);
  }
  else {
    *process_stderr_read  = NULL;
    *process_stderr_write = duplicate_handle(GetStdHandle(STD_ERROR_HANDLE));
  }

  // Close any pipe ends that we may have passed into the child.
  if (stdin_read   != NULL) CloseHandle(stdin_read);
  if (stdout_write != NULL) CloseHandle(stdout_write);
  if (stderr_write != NULL) CloseHandle(stderr_write);
}

//- env_vars: If not null, a string with the setting of all environment variables packed
//  into it. Format is "MYVAR1=MYVALUE1\0MYVAR2=MYVALUE2\0\0". Note the extra \0 at the end,
//  which signifies the end of the environment variable list itself. 
stz_int launch_process(stz_byte* command_line,
                       stz_int input, stz_int output, stz_int error,
                       stz_byte* working_dir, stz_byte* env_vars, Process* process) {
  // Set up our STDIN, STDOUT, and STDERR  
  HANDLE stdin_read, stdin_write,
         stdout_read, stdout_write,
         stderr_read, stderr_write;
  setup_file_handles(input, output, error,
      &stdin_read, &stdin_write,
      &stdout_read, &stdout_write,
      &stderr_read, &stderr_write);

  // Now that we have our handles, set up STARTUPINFO so that the child process will use them
  STARTUPINFO start_info;
  ZeroMemory(&start_info, sizeof(STARTUPINFO));
  start_info.cb = sizeof(STARTUPINFO);
  start_info.hStdInput  = stdin_read;
  start_info.hStdOutput = stdout_write;
  start_info.hStdError  = stderr_write;
  start_info.dwFlags |= STARTF_USESTDHANDLES;

  // Zero out our process information in ancipication of CreateProcess
  // populating it, and then launch our process.
  PROCESS_INFORMATION proc_info;
  ZeroMemory(&proc_info, sizeof(PROCESS_INFORMATION));
  BOOL success = CreateProcess(
      /* lpApplicationName    */ NULL,
      /* lpCommandLine        */ (LPSTR)command_line,
      /* lpProcessAttributes  */ NULL,
      /* lpThreadAttributes   */ NULL,
      /* bInheritHandles      */ TRUE,
      /* dwCreationFlags      */ 0,
      /* lpEnvironment        */ (LPSTR)env_vars,
      /* lpCurrentDirectory   */ (LPSTR)working_dir,
      /* lpStartupInfo        */ &start_info,
      /* lpProcessInformation */ &proc_info);

  if (success) {
    // Populate process with the relevant info
    process->pid = (stz_long)proc_info.dwProcessId;
    process->pipeid = -1; // -1 signals we didn't create named pipes for this process
    process->handle = (void*)proc_info.hProcess;
    process->in  = file_from_handle(stdin_write, FT_WRITE);
    process->out = file_from_handle(stdout_read, FT_READ);
    process->err = file_from_handle(stderr_read, FT_READ);

    // Close the handles we passed into the child process
    if (stdin_read   != NULL) CloseHandle(stdin_read);
    if (stdout_write != NULL) CloseHandle(stdout_write);
    if (stderr_write != NULL) CloseHandle(stderr_write);

    // Close these left-over handles to the process
    // Leave the process handle (proc_info.hProcess) open, so that we
    // can use it for retrieve_state. It will be closed from the Stanza side.
    CloseHandle(proc_info.hThread);    
  }

  return success ? 0 : -1;
}

//C function to close the handle. Called by Stanza.
void close_process_handle (void* handle) {
  CloseHandle(handle);
}
