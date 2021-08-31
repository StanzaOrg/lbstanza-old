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

static char* allocating_sprintf(const char *restrict fmt, ...) {
  char* string;
  size_t bytes_needed;
  va_list args;

  va_start(args, fmt);

  bytes_needed = vsnprintf(NULL, 0, fmt, args);
  string = (char*)stz_malloc(bytes_needed + 1);
  vsprintf(string, fmt, args);

  va_end(args);

  return string;
}

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

// Takes a NULL-terminated list of strings and concatenates them using ' ' as a
// separator. This is necessary because CreateProcess doesn't take an argument
// list, but instead expects a string containing the current command line.
static char* create_command_line_from_argv(const stz_byte** argv) {
  char* ret;
  char* cursor;
  bool first;
  size_t total_length;

  total_length = 0;
  for (const stz_byte** arg = argv; *arg != NULL; ++arg) {
    total_length += strlen(C_CSTR(*arg)) + 3; // +1 for the trailing ' ' or '\0',
                                              // +2 for the enclosing '"'s
  }

  ret = (char*)stz_malloc(total_length);

  cursor = ret;
  first = true;
  for (const stz_byte** arg = argv; *arg != NULL; ++arg) {
    if (!first) *cursor++ = ' ';
    first = false;

    *cursor++ = '"';
    for (const stz_byte* c = *arg; *c != '\0'; ++c) {
      *cursor++ = (char)*c;
    }
    *cursor++ = '"';
  }
  *cursor++ = '\0';

  return ret;
}

// Split a given string into a list of strings according to a delimeter
static StringList* split_string(const char* str, char delimeter) {
  StringList* list;
  const char *start, *end;
  char *current;

  list = make_stringlist(1);
  start = str;
  while ((end = strchr(start, delimeter)) != NULL) {
    current = (char*)stz_malloc(end - start + 1);
    strncpy(current, start, end - start);
    current[end - start] = '\0';

    stringlist_add(list, STZ_CSTR(current));
    stz_free(current);

    start = ++end;
  }

  return list;
}

static bool is_executable_path(const char* path) {
  DWORD binary_type;
  return GetBinaryTypeA(path, &binary_type) &&
    (binary_type == SCS_32BIT_BINARY ||
     binary_type == SCS_64BIT_BINARY);
}

// Iterate over all the directories in PATH and determine in which directory
// there exists an executable with the given file name. If one is found, return
// it, otherwise return NULL.
static LPSTR find_candidate_in_path(const char* file) {
  StringList* candidates;
  const char *candidate_dir;
  char *candidate_path, *candidate_exe_path, *ret;

  ret = NULL;

  candidates = split_string(getenv("PATH"), ';');
  for (int i = 0; i < candidates->n; ++i) {
    candidate_dir = C_CSTR(candidates->strings[i]);

    // First check if the given file as-is is an executable path
    candidate_path = allocating_sprintf("%s\\%s", candidate_dir, file);
    if (is_executable_path(candidate_path)) {
      ret = candidate_path;
      break;
    }
    stz_free(candidate_path);

    // Otherwise, check if the file with ".exe" appended is an executable path
    candidate_exe_path = allocating_sprintf("%s\\%s.exe", candidate_dir, file);
    if (is_executable_path(candidate_exe_path)) {
      ret = candidate_exe_path;
      break;
    }
    stz_free(candidate_exe_path);
  }

  free_stringlist(candidates);
  return ret;
}

static bool contains_path_separator(const char* file) {
  // Windows accepts both '/' and '\' in most APIs
  return strchr(file, '\\') != NULL ||
         strchr(file, '/')  != NULL;
}

// Accept a path which may be relative (e.g. contains '..' or '.') and contain
// forward-slashes, and convert it to an absolute path with back-slashes
static char* normalize_path(const char* file) {
  char* normalized_path;

  normalized_path = (char*)malloc(MAX_PATH * sizeof(char));
  if (!GetFullPathName(file, MAX_PATH, normalized_path, NULL)) {
    return NULL;
  }

  return normalized_path;
}

void initialize_launcher_process (void) { /* stub */ }

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

stz_int launch_process(const stz_byte* file, const stz_byte** argvs,
                       stz_int input, stz_int output, stz_int error,
                       stz_int pipeid, Process* process) {
  const char* file_str;
  LPSTR command_line, executable;
  PROCESS_INFORMATION proc_info;
  STARTUPINFO start_info;
  HANDLE stdin_read, stdin_write,
         stdout_read, stdout_write,
         stderr_read, stderr_write;
  BOOL success;

  // Create a command line using the given argv. This is necessary because
  // Window's CreateProcess expects a full command-line (process name +
  // arguments), rather than passing in a list of args (like exec* on POSIX)
  command_line = (LPSTR)create_command_line_from_argv(argvs);

  // Find an executable path using the given file. If we were given a path,
  // normalize it, in case it is relative or has forward-slashes If it's a
  // file, find the path of a corresponding executable on PATH
  file_str = C_CSTR(file);
  executable = contains_path_separator(file_str)
    ? normalize_path(file_str)
    : find_candidate_in_path(file_str);

  if (command_line == NULL || executable == NULL) {
    success = FALSE;
    goto END;
  }

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
      /* lpApplicationName    */ executable,
      /* lpCommandLine        */ command_line,
      /* lpProcessAttributes  */ NULL,
      /* lpThreadAttributes   */ NULL,
      /* bInheritHandles      */ TRUE,
      /* dwCreationFlags      */ 0,
      /* lpEnvironment        */ NULL,
      /* lpCurrentDirectory   */ NULL,
      /* lpStartupInfo        */ &start_info,
      /* lpProcessInformation */ &proc_info);

END:
  stz_free(executable);
  stz_free(command_line);

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

stz_int delete_process_pipes (FILE* input, FILE* output, FILE* error, stz_int pipeid) {
  return 0;
}
