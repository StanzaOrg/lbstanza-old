#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#ifdef PLATFORM_WINDOWS
  #include <fcntl.h>
  #include <io.h>
  #include <windows.h>
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  typedef int SOCKET;
#endif

static FILE* debug_adapter_log;
static pthread_mutex_t send_lock;
static const char* debug_adapter_path;

static inline char* get_absolute_path(const char* s) {
  // Use GetFullPathName on Windows
  return realpath(s, NULL);
}
static inline void free_path(const char* s) {
  free((char*)s);
}

// Check if i'th argument is followed by a value. The value must not start with "--".
static inline bool has_arg_value(int i, const int argc, char* const argv[]) {
  if (++i >= argc) return false;
  const char* s = argv[i];
  if (s[0] == '-' && s[1] == '-') return false;
  return true;
}

// Report an error if i'th argument is not followed by a value. The value must not start with "--".
static inline void expect_arg_value(int i, const int argc, char* const argv[]) {
  if (!has_arg_value(i, argc, argv)) {
    fprintf(stderr, "Argument %s must be followed by a value\n", argv[i]);
    exit(EXIT_FAILURE);
  }
}

// Search argv backwards for the option starting with --name.
// Return the index of the option or 0 if the option is not found.
// Note: argv[0] is the program path.
static inline int find_last_arg(const char* name, int argc, char* const argv[]) {
  while (--argc > 0) {
    const char* s = argv[argc];
    if (s[0] == '-' && s[1] == '-' && strcmp(s+2, name) == 0)
      break;
  }
  return argc;
}

// Search for the arg by name. Verify that it is followed by a value.
static int find_last_arg_with_value(const char* name, int argc, char* const argv[]) {
  const int i = find_last_arg(name, argc, argv);
  if (i) expect_arg_value(i, argc, argv);
  return i;
}

// I/O interface and its implementation over files and sockets.
// Note: I am not sure why is this necessary in LLDB.
static ssize_t (*debug_adapter_read) (char* data, ssize_t length);
static ssize_t (*debug_adapter_write) (const char* data, ssize_t length);

static SOCKET debug_adapter_socket;
static ssize_t read_from_socket(char* data, ssize_t length) {
  const ssize_t bytes_received = recv(debug_adapter_socket, data, length, 0);
#ifdef PLATFORM_WINDOWS
  errno = WSAGetLastError();
#endif
  return bytes_received;
}
static ssize_t write_to_socket(const char* data, ssize_t length) {
  const ssize_t bytes_sent = send(debug_adapter_socket, data, length, 0);
#ifdef PLATFORM_WINDOWS
  errno = WSAGetLastError();
#endif
  return bytes_sent;
}

static int debug_adapter_input_fd;
static int debug_adapter_output_fd;
static ssize_t read_from_file(char* data, ssize_t length) {
  return read(debug_adapter_input_fd, data, length);
}
static ssize_t write_to_file(const char* data, ssize_t length) {
  return write(debug_adapter_output_fd, data, length);
}

static void write_full(const char* data, ssize_t length) {
  while (length) {
    const ssize_t bytes_written = (*debug_adapter_write)(data, length);
    if (bytes_written < 0) {
      if (errno == EINTR || errno == EAGAIN)
        continue;
      if (debug_adapter_log)
        fprintf(debug_adapter_log, "Error writing data\n");
      return;
    }
    assert(((unsigned)bytes_written) <= ((unsigned)length));
    data += bytes_written;
    length -= bytes_written;
  }
}

static bool read_full(char* data, ssize_t length) {
  while (length) {
    const ssize_t bytes_read = (*debug_adapter_read)(data, length);
    if (bytes_read == 0) {
      if (debug_adapter_log)
        fprintf(debug_adapter_log, "End of file (EOF) reading from input file\n");
      return false;
    }
    if (bytes_read < 0) {
      if (errno == EINTR || errno == EAGAIN)
        continue;
      if (debug_adapter_log)
        fprintf(debug_adapter_log, "Error reading data\n");
      return false;
    }

    assert(((unsigned)bytes_read) <= ((unsigned)length));
    data += bytes_read;
    length -= bytes_read;
  }
  return true;
}

static void write_string(const char* s) {
  write_full(s, strlen(s));
}
static inline void write_unsigned(const unsigned long long v) {
  char buffer[22];
  snprintf(buffer, sizeof buffer, "%llu", v);
  write_string(buffer);
}

enum {
  #define DEF(name) JSON_FIELD_##name,
  #include "jsonfields.inc"
  JSON_FIELDS_COUNT
};
static const char* const JSON_field_names[] = {
  #define DEF(name) #name,
  #include "jsonfields.inc"
  NULL
};
static inline const char* JSON_field_name(const unsigned field_id) {
  assert(field_id < JSON_FIELDS_COUNT);
  return JSON_field_names[field_id];
}
static inline int JSON_field_id(const char* name) {
  for (const char* const* p = JSON_field_names; *p; p++)
    if (strcmp(*p, name) == 0)
      return p - JSON_field_names;
  return -1;
}

typedef enum {
  JSON_Integer,
  JSON_Object,
  JSON_String
} JSON_FIELD_TYPE;

typedef struct JSON_FIELD {
  struct JSON_FIELD* next;
  int key;
  JSON_FIELD_TYPE type;
  ssize_t size;
  char value[];
} JSON_Field;

static inline JSON_Field* allocate_key_value_pair(JSON_FIELD_TYPE type, const void* value, ssize_t size) {
  JSON_Field* field = malloc(sizeof(JSON_Field) + size);
  //TODO: handle possible OOME
  field->type = type;
  field->size = size;
  memcpy(field->value, value, size);
  return field;
}
static JSON_Field* allocate_key_integer_pair(const int64_t value) {
  return allocate_key_value_pair(JSON_Integer, &value, sizeof value);
}
static JSON_Field* allocate_key_object_pair(const void* value) {
  return allocate_key_value_pair(JSON_Object, &value, sizeof value);
}
static JSON_Field* allocate_key_text_pair(const char* value, ssize_t length) {
  //TODO: handle UTF8 value
  return allocate_key_value_pair(JSON_String, value, length);
}

typedef struct {
  JSON_Field* fields;
} JSON;
static void inline JSON_initialize(JSON* object) {
  object->fields = NULL;
}
static void JSON_destroy(JSON* object) {
  for (JSON_Field* p = object->fields; p;) {
    JSON_Field* next = p->next;
    free(p);
    p = next;
  }
}
static inline const JSON_Field* JSON_find_field(const JSON* object, int key) {
  const JSON_Field* p = object->fields;
  while (p && p->key != key)
    p = p->next;
  return p;
}
static inline void JSON_insert_field(JSON* object, int key, JSON_Field* field) {
  assert(!JSON_find_field(object, key));
  field->key = key;
  field->next = object->fields;
  object->fields = field;
}
static void JSON_set_integer_field(JSON* object, int key, const int64_t value) {
  JSON_insert_field(object, key, allocate_key_integer_pair(value));
}
static void JSON_set_text_field(JSON* object, int key, const char* text, ssize_t length) {
  JSON_insert_field(object, key, allocate_key_text_pair(text, length));
}
static inline void JSON_set_string_field(JSON* object, int key, const char* value) {
  JSON_set_text_field(object, key, value, strlen(value));
}
static const char* JSON_set_object_field(JSON* object, int key, const JSON* value) {
  JSON_insert_field(object, key, allocate_key_object_pair(value));
}

typedef struct {
  ssize_t length;
  ssize_t capacity;
  char* data;
  int indent;
} JSBuilder;
static inline void JSBuilder_initialize(JSBuilder* builder) {
  builder->indent = 0;
  builder->length = 0;
  builder->capacity = 16*1024; // Initial buffer size
  builder->data = malloc(builder->capacity);
  //TODO: handle possible OOME
}
static inline void JSBuilder_destroy(JSBuilder* builder) {
  free(builder->data);
}
static void JSBuilder_send_and_destroy(JSBuilder* builder) {
  pthread_mutex_lock(&send_lock);

  char* data = builder->data;
  const ssize_t length = builder->length;
  write_string("Content-Length: ");
  write_unsigned(length);
  write_string("\r\n\r\n");
  write_full(data, length);

  if (debug_adapter_log) {
    fprintf(debug_adapter_log, "<--\nContent-Length: %llu\n\n", (unsigned long long)length);
    fwrite(data, length, 1, debug_adapter_log);
    fprintf(debug_adapter_log, "\n");
  }

  pthread_mutex_unlock(&send_lock);
  free(data);
}

static void JSBuilder_ensure_capacity(JSBuilder* builder, ssize_t size) {
  ssize_t capacity = builder->capacity;
  const ssize_t length = builder->length;
  const ssize_t required_capacity = length + size;
  if (required_capacity <= capacity) return;

  while (capacity < required_capacity)
    capacity <<= 1;
  builder->capacity = capacity;

  char* data = builder->data;
  builder->data = memcpy(malloc(capacity), data, length);
  //TODO: handle possible OOM
  free(data);
}
static char* JSBuilder_allocate(JSBuilder* builder, ssize_t size) {
  JSBuilder_ensure_capacity(builder, size);
  char* data = builder->data + builder->length;
  builder->length += size;
  return data;
}
static void JSBuilder_append_char(JSBuilder* builder, char c) {
  JSBuilder_allocate(builder, 1)[0] = c;
}
static void JSBuilder_append_text(JSBuilder* builder, const char* data, ssize_t length) {
  if (length)
    memcpy(JSBuilder_allocate(builder, length), data, length);
}
static void JSBuilder_append_string(JSBuilder* builder, const char* s) {
  JSBuilder_append_text(builder, s, strlen(s));
}

static void JSBuilder_append_quotes(JSBuilder* builder) {
  JSBuilder_append_char(builder, '\"');
}
static void JSBuilder_write_quoted_raw_string(JSBuilder* builder, const char* s) {
  JSBuilder_append_quotes(builder);
  JSBuilder_append_string(builder, s);
  JSBuilder_append_quotes(builder);
}
static inline char hex_nybble(char c) {
  return "0123456789ABCDEF"[c & 0xF];
}
static void JSBuilder_write_quoted_text(JSBuilder* builder, const char* data, ssize_t length) {
  JSBuilder_append_quotes(builder);
  for (const char* limit = data + length;;) {
    const char* p = data;
    for (; p < limit; p++) {
      const unsigned char c = *p;
      if (c < ' ' || c == '\"' || c == '\\')
        break;
    }
    JSBuilder_append_text(builder, data, p - data);
    data = p;
    if (data == limit) break;

    char* s = JSBuilder_allocate(builder, 2);
    *s++ = '\\';

    const unsigned char c = *data++;
    switch (c) {
      case '\"': *s = '\"'; break;
      case '\\': *s = '\\'; break;
      case '\t': *s = 't'; break;
      case '\n': *s = 'n'; break;
      case '\r': *s = 'r'; break;
      default:
        *s = 'x';
        s = JSBuilder_allocate(builder, 2);
        s[1] = hex_nybble(c);
        s[0] = hex_nybble(c >> 4);
    }
  }
  JSBuilder_append_quotes(builder);
}

enum { JSIndentStep = 2 };
static inline void JSBuilder_indent(JSBuilder* builder) {
  builder->indent += JSIndentStep;
}
static inline void JSBuilder_unindent(JSBuilder* builder) {
  builder->indent -= JSIndentStep;
  assert(builder->indent >= 0);
}
static void JSBuilder_newline(JSBuilder* builder) {
  int chars_to_append = builder->indent + 1;
  char* data = JSBuilder_allocate(builder, chars_to_append);
  *data++ = '\n';
  while (--chars_to_append > 0)
    *data++ = ' ';
}

static inline void JSBuilder_next(JSBuilder* builder, bool* next) {
  if (*next)
    JSBuilder_append_char(builder, ',');
  *next = true;
  JSBuilder_newline(builder);
}

static void JSBuilder_write_field(JSBuilder* builder, bool* next, const char* name) {
  JSBuilder_next(builder, next);
  JSBuilder_write_quoted_raw_string(builder, name);
  JSBuilder_append_string(builder, ": ");
}
static void JSBuilder_write_raw_string_field(JSBuilder* builder, bool* next, const char* name, const char* value) {
  JSBuilder_write_field(builder, next, name);
  JSBuilder_write_quoted_raw_string(builder, value);
}

static inline void JSBuilder_structure_begin(JSBuilder* builder, char brace) {
  JSBuilder_append_char(builder, brace);
  JSBuilder_indent(builder);
}
static inline void JSBuilder_structure_end(JSBuilder* builder, char brace) {
  JSBuilder_unindent(builder);
  JSBuilder_newline(builder);
  JSBuilder_append_char(builder, brace);
}
static void JSBuilder_object_begin(JSBuilder* builder) {
  JSBuilder_structure_begin(builder, '{');
}
static void JSBuilder_object_end(JSBuilder* builder) {
  JSBuilder_structure_end(builder, '}');
}
static void JSBuilder_array_begin(JSBuilder* builder) {
  JSBuilder_structure_begin(builder, '[');
}
static void JSBuilder_array_end(JSBuilder* builder) {
  JSBuilder_structure_end(builder, ']');
}

static void JSBuilder_initialize_event(JSBuilder* builder, const char* name) {
  JSBuilder_initialize(builder);
  JSBuilder_object_begin(builder);
  bool next = false;
  JSBuilder_write_field(builder, &next, "seq"); JSBuilder_append_char(builder, '0');
  JSBuilder_write_raw_string_field(builder, &next, "type", "event");
  JSBuilder_write_raw_string_field(builder, &next, "event", name);
  JSBuilder_write_field(builder, &next, "body");
  JSBuilder_object_begin(builder);
}
static void JSBuilder_send_and_destroy_event(JSBuilder* builder) {
  JSBuilder_object_end(builder); // event
  JSBuilder_object_end(builder); // body
  JSBuilder_send_and_destroy(builder);
}

// "OutputEvent": {
//   "allOf": [ { "$ref": "#/definitions/Event" }, {
//     "type": "object",
//     "description": "Event message for 'output' event type. The event
//                     indicates that the target has produced some output.",
//     "properties": {
//       "event": {
//         "type": "string",
//         "enum": [ "output" ]
//       },
//       "body": {
//         "type": "object",
//         "properties": {
//           "category": {
//             "type": "string",
//             "description": "The output category. If not specified,
//                             'console' is assumed.",
//             "_enum": [ "console", "stdout", "stderr", "telemetry" ]
//           },
//           "output": {
//             "type": "string",
//             "description": "The output to report."
//           },
//           "variablesReference": {
//             "type": "number",
//             "description": "If an attribute 'variablesReference' exists
//                             and its value is > 0, the output contains
//                             objects which can be retrieved by passing
//                             variablesReference to the VariablesRequest."
//           },
//           "source": {
//             "$ref": "#/definitions/Source",
//             "description": "An optional source location where the output
//                             was produced."
//           },
//           "line": {
//             "type": "integer",
//             "description": "An optional source location line where the
//                             output was produced."
//           },
//           "column": {
//             "type": "integer",
//             "description": "An optional source location column where the
//                             output was produced."
//           },
//           "data": {
//             "type":["array","boolean","integer","null","number","object",
//                     "string"],
//             "description": "Optional data to report. For the 'telemetry'
//                             category the data will be sent to telemetry, for
//                             the other categories the data is shown in JSON
//                             format."
//           }
//         },
//         "required": ["output"]
//       }
//     },
//     "required": [ "event", "body" ]
//   }]
// }
typedef enum {
  CONSOLE,
  STDOUT,
  STDERR,
  TELEMETRY
} OutputType;

static inline const char* output_category(const OutputType type) {
  static const char* const names[] = {
    "console",
    "stdout",
    "stderr",
    "telemetry"
  };
  return names[type];
}

static void send_output(const OutputType out, const char* data, ssize_t length) {
  assert(length > 0);
  JSBuilder builder;
  JSBuilder_initialize_event(&builder, "output");
  bool next = false;
  JSBuilder_write_field(&builder, &next, "output");
  JSBuilder_write_quoted_text(&builder, data, length);
  JSBuilder_write_raw_string_field(&builder, &next, "category", output_category(out));
  JSBuilder_send_and_destroy_event(&builder);
}

static void* redirect_output_loop(void* args) {
  const int read_fd = ((const int*)args)[0];
  const OutputType out = (OutputType)(((const int*)args)[1]);
  while (true) {
    char buffer[4096];
    const int bytes_read = read(read_fd, &buffer, sizeof buffer);
    if (bytes_read > 0)
      send_output(out, buffer, bytes_read);
  }
  return NULL;
}
static const char* redirect_fd(int fd, const OutputType out) {
  static char error_buffer[256];
  int new_fd[2];
#ifdef PLATFORM_WINDOWS
  if (_pipe(new_fd, 4096, O_TEXT) == -1) {
#else
  if (pipe(new_fd) == -1) {
#endif
    const int error = errno;
    snprintf(error_buffer, sizeof error_buffer, "Couldn't create new pipe for fd %d. %s", fd, strerror(error));
    return error_buffer;
  }
  if (dup2(new_fd[1], fd) == -1) {
    const int error = errno;
    snprintf(error_buffer, sizeof error_buffer, "Couldn't override the fd %d. %s", fd, strerror(error));
    return error_buffer;
  }

  int args[2] = {new_fd[0], (int)out};
  pthread_t thread;
  if (pthread_create(&thread, NULL, &redirect_output_loop, &args)) {
    const int error = errno;
    snprintf(error_buffer, sizeof error_buffer, "Couldn't create the redirect thread for fd %d. %s", fd, strerror(error));
    return error_buffer;
  }
  return NULL;
}

static void redirect_output(FILE* file, const OutputType out) {
  const char* error = redirect_fd(fileno(file), out);
  if (error) {
    if (debug_adapter_log)
      fprintf(debug_adapter_log, "%s\n", error);
    send_output(STDERR, error, strlen(error));
  }
}

static inline void launch_target_in_terminal(const char* comm_file, const int argc, char* const argv[]) {
  fprintf(stderr, "launch_target_in_terminal is not implemented\n");
  exit(EXIT_FAILURE);
}

static inline int debug(void) {
  // redirect_output(stdout, STDOUT);
  // redirect_output(stderr, STDERR);
  // main loop
  return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
  // setvbuf(stdout, NULL, _IOLBF, BUFSIZ);

  // Allocate and compute absolute path to the adapter
  debug_adapter_path = get_absolute_path(argv[0]);

  int launch_target_pos = find_last_arg_with_value("launch-target", argc, argv);
  if (launch_target_pos) {
    const int comm_file_pos = find_last_arg_with_value("comm-file", launch_target_pos, argv);
    if (!comm_file_pos) {
      fprintf(stderr, "--launch-target option requires --comm-file to be specified\n");
      return EXIT_FAILURE;
    }
    ++launch_target_pos;
    const char* comm_path = argv[comm_file_pos + 1];
    char* const* launch_target_argv = argv + launch_target_pos;
    const int launch_target_argc = argc - launch_target_pos;
    launch_target_in_terminal(comm_path, launch_target_argc, launch_target_argv);
  }

#ifndef PLATFORM_WINDOWS
  if (find_last_arg("wait-for-debugger", argc, argv)) {
    printf("Paused waiting for debugger to attach (pid = %i)...\n", getpid());
    pause();
  }
#endif

  const int port_pos = find_last_arg_with_value("port", argc, argv);
  if (port_pos) {
    const char* port_arg = argv[port_pos + 1];
    char* remainder;
    int port = strtoul(port_arg, &remainder, 0); // Ordinary C notation
    if (*remainder) {
      fprintf(stderr, "'%s' is not a valid port number.\n", port_arg);
      return EXIT_FAILURE;
    }

    printf("Listening on port %i...\n", port);
    SOCKET tmpsock = socket(AF_INET, SOCK_STREAM, 0);
    if (tmpsock < 0) {
      if (debug_adapter_log)
        fprintf(debug_adapter_log, "error: opening socket (%s)\n", strerror(errno));
      return EXIT_FAILURE;
    }

    {
      struct sockaddr_in serv_addr;
      memset(&serv_addr, 0, sizeof serv_addr);
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      serv_addr.sin_port = htons(port);
      if (bind(tmpsock, (struct sockaddr*)&serv_addr, sizeof serv_addr) < 0) {
        if (debug_adapter_log)
          fprintf(debug_adapter_log, "error: binding socket (%s)\n", strerror(errno));
        return EXIT_FAILURE;
      }
    }
    listen(tmpsock, 5);

    SOCKET sock;
    do {
      struct sockaddr_in cli_addr;
      socklen_t cli_addr_len = sizeof cli_addr;
      sock = accept(tmpsock, (struct sockaddr*)&cli_addr, &cli_addr_len);
    } while (sock < 0 && errno == EINTR);
    if (sock < 0) {
      if (debug_adapter_log)
        fprintf(debug_adapter_log, "error: accepting socket (%s)\n", strerror(errno));
      return EXIT_FAILURE;
    }
    debug_adapter_socket = sock;
    debug_adapter_read = &read_from_socket;
    debug_adapter_write = &write_to_socket;
  } else {
    debug_adapter_input_fd = fileno(stdin);
    debug_adapter_output_fd = fileno(stdout);
    debug_adapter_read = &read_from_file;
    debug_adapter_write = &write_to_file;
  }

  if (pthread_mutex_init(&send_lock, NULL)) {
    if (debug_adapter_log)
      fprintf(debug_adapter_log, "error: initializing mutex (%s)\n", strerror(errno));
    return EXIT_FAILURE;
  }

  // Initialize debugger
  const int ret_code = debug();
  // Terminate debugger

 // fflush(stdout);
 // sleep(1);
  free_path(debug_adapter_path);
  return ret_code;
}