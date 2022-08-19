#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static const char* current_error(void) {
  return strerror(errno);
}

static FILE* debug_adapter_log;
static pthread_mutex_t log_lock;
static void log_printf(const char* fmt, ...) {
  if (debug_adapter_log) {
    pthread_mutex_lock(&log_lock);

    va_list args;
    va_start(args, fmt);
    vfprintf (debug_adapter_log, fmt, args);
    va_end(args);

    pthread_mutex_unlock(&log_lock);
  }
}
static void log_packet(const char* prefix, const char* data, ssize_t length) {
  if (debug_adapter_log) {
    pthread_mutex_lock(&log_lock);

    fprintf(debug_adapter_log, "\n%s\nContent-Length: %" PRIu64 "\n\n", prefix, (uint64_t)length);
    fwrite(data, length, 1, debug_adapter_log);
    fprintf(debug_adapter_log, "\n");

    pthread_mutex_unlock(&log_lock);
  }
}

typedef struct {
  ssize_t length;
  ssize_t capacity;
  char** data;
} StringVector;
static void StringVector_initialize(StringVector* vector) {
  vector->length = 0;
  vector->capacity = 16;
  vector->data = malloc(vector->capacity * sizeof(char*));
  //TODO: handle possible OOM
}
static void StringVector_destroy(StringVector* vector) {
  char** data = vector->data;
  int length = vector->length;
  for (int i = 0; i < length; i++)
    free(data[i]);
  free(data);
}
static inline void StringVector_reset(StringVector* vector) {
  vector->length = 0;
}
static void StringVector_append(StringVector* vector, const char* s) {
  if (vector->length == vector->capacity) {
    vector->capacity <<= 1;
    vector->data = realloc(vector->data, vector->capacity * sizeof(char*));
    //TODO: handle possible OOM
  }
  vector->data[vector->length++] = strdup(s);
  //TODO: handle possible OOM
}

static char* program_path;
static pid_t program_pid;
static bool sent_terminated_event;
static pthread_mutex_t send_lock;
static const char* debug_adapter_path;

static inline char* get_absolute_path(const char* s) {
  // Use GetFullPathName on Windows
  return realpath(s, NULL);
}
static inline void free_path(const char* s) {
  free((char*)s);
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
      log_printf("Error writing data\n");
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
      log_printf("End of file (EOF) reading from input file\n");
      return false;
    }
    if (bytes_read < 0) {
      if (errno == EINTR || errno == EAGAIN)
        continue;
      log_printf("Error reading data\n");
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
static inline void write_packet(const char* data, const ssize_t length) {
  pthread_mutex_lock(&send_lock);

  write_string("Content-Length: ");
  write_unsigned(length);
  write_string("\r\n\r\n");
  write_full(data, length);

  log_packet("<--", data, length);

  pthread_mutex_unlock(&send_lock);
}

static bool read_expected(const char* expected) {
  char buffer[32];
  ssize_t length = strlen(expected);
  return read_full(buffer, length) && memcmp(buffer, expected, length) == 0;
}
static inline ssize_t read_unsigned(void) {
  char buffer[32];
  int count = 0;
  char c;
  do {
    if (!read_full(&c, sizeof c)) return 0;
    if (count < sizeof buffer)
      buffer[count++] = c;
  } while (c != '\r');

  if (!read_full(&c, sizeof c)) return 0;
  if (c == '\n') {
    if (count > sizeof buffer) return 0;
    char* remainder;
    unsigned long value = strtoul(buffer, &remainder, 10); // Decimal only
    if (*remainder == '\r') return value;
  }
  return 0;
}

static inline ssize_t read_packet(char** p_data) {
  *p_data = NULL;
  if (!read_expected("Content-Length: ")) {
    log_printf("Content-Length not found\n");
    return 0;
  }
  ssize_t length = read_unsigned();
  if (!read_expected("\r\n")) return 0;
  if (!length) {
    log_printf("Zero-length content\n");
    return 0;
  }

  char* data = malloc(length + 1); // For extra `\0' at the end
  *p_data = data;
  if (!data) {
    log_printf("Cannot allocate %ld-byte content\n", (long) length);
    return 0;
  }
  if (!read_full(data, length)) {
    log_printf("Failed to read %ld-byte content\n", (long) length);
    return 0;
  }
  data[length] = 0;
  log_packet("-->", data, length);
  return length;
}

typedef enum {
  JS_NULL,
  JS_BOOLEAN,
  JS_INTEGER,
  JS_DOUBLE,
  JS_STRING,
  JS_OBJECT,
  JS_ARRAY
} JSValueKind;

typedef struct {
  struct JS_FIELD* fields;
} JSObject;

typedef struct {
  ssize_t length;
  ssize_t capacity;
  struct JS_VALUE* data;
} JSArray;

typedef struct JS_VALUE {
  JSValueKind kind;
  union {
    bool b;
    int64_t i;
    double d;
    const char* s;
    JSObject o;
    JSArray a;
  } u;
} JSValue;

// Initialize the value as JS_NULL. Later it can be safely
// re-initialized as any other kind.
static inline JSValue* JSValue_initialize(JSValue* value) {
  value->kind = JS_NULL;
  return value;
}
static void JSValue_destroy(JSValue* value);

static inline void JSValue_make_boolean(JSValue* value, bool b) {
  value->kind = JS_BOOLEAN;
  value->u.b = b;
}
static inline void JSValue_make_integer(JSValue* value, int64_t i) {
  value->kind = JS_INTEGER;
  value->u.i = i;
}
static inline void JSValue_make_double(JSValue* value, double d) {
  value->kind = JS_DOUBLE;
  value->u.d = d;
}
// Strings are zero-terminated. Consider (data, length) pairs if strings need to contain `\0's.
// JSValue is not resposible for string deallocation.
static inline void JSValue_make_string(JSValue* value, const char* s) {
  value->kind = JS_STRING;
  value->u.s = s;
}

static inline JSArray* JSArray_initialize(JSArray* array) {
  array->length = 0;
  array->capacity = 16; // Initial array size
  array->data = malloc(array->capacity * sizeof(JSValue));
  //TODO: handle possible OOME
  return array;
}
static inline void JSArray_destroy(JSArray* array) {
  JSValue* data = array->data;
  const JSValue* limit = data + array->length;
  JSValue* p = data;
  for (JSValue* p = data; p < limit; p++)
    JSValue_destroy(p);
  free(data);
}
static inline JSArray* JSValue_make_array(JSValue* value) {
  value->kind = JS_ARRAY;
  return JSArray_initialize(&value->u.a);
}
static inline JSValue* JSArray_append(JSArray* array) {
  if (array->length == array->capacity) {
    array->capacity <<= 1;
    array->data = realloc(array->data, array->capacity * sizeof(JSValue));
    //TODO: handle possible OOM
  }
  // Only initialized values can be destroyed.
  return JSValue_initialize(array->data + array->length++);
}

typedef struct JS_FIELD {
  struct JS_FIELD* next;
  const char* name;
  JSValue value;
} JSField;
static JSField* JSField_find(JSObject* object, const char* name) {
  JSField* field;
  for (field = object->fields; field && strcmp(field->name, name); field = field->next);
  return field;
}
static inline JSField* JSField_create(JSObject* object, const char* name) {
  JSField* field = JSField_find(object, name);
  if (field) {
    JSValue_destroy(&field->value);
  } else {
    field = malloc(sizeof *field);
    //TODO: handle possible OOM
    field->name = name;
    field->next = object->fields;
    object->fields = field;
  }
  JSValue_initialize(&field->value);
  return field;
}
static inline JSField* JSField_destroy(JSField* field) {
  JSField* next = field->next;
  JSValue_destroy(&field->value);
  free(field);
  return next;
}
static const JSField* JSObject_get_field(const JSObject* object, const char* name, const JSValueKind kind) {
  if (object) {
    const JSField* field = JSField_find((JSObject*)object, name);
    if (field && field->value.kind == kind)
      return field;
  }
  return NULL;
}
static const char* JSObject_get_string_field(const JSObject* object, const char* name) {
  const JSField* field = JSObject_get_field(object, name, JS_STRING);
  return field ? field->value.u.s : NULL;
}
static const JSObject* JSObject_get_object_field(const JSObject* object, const char* name) {
  const JSField* field = JSObject_get_field(object, name, JS_OBJECT);
  return field ? &field->value.u.o : NULL;
}
static const JSArray* JSObject_get_array_field(const JSObject* object, const char* name) {
  const JSField* field = JSObject_get_field(object, name, JS_ARRAY);
  return field ? &field->value.u.a : NULL;
}
static int64_t JSObject_get_integer_field(const JSObject* object, const char* name, int64_t default_value) {
  const JSField* field = JSObject_get_field(object, name, JS_INTEGER);
  return field ? field->value.u.i : default_value;
}
static int64_t JSObject_get_bool_field(const JSObject* object, const char* name, bool default_value) {
  const JSField* field = JSObject_get_field(object, name, JS_BOOLEAN);
  return field ? field->value.u.b : default_value;
}

static inline JSObject* JSObject_initialize(JSObject* object) {
  object->fields = NULL;
  return object;
}
static inline void JSObject_destroy(JSObject* object) {
  for (JSField* field = object->fields; field; field = JSField_destroy(field));
}
static inline JSObject* JSValue_make_object(JSValue* value) {
  value->kind = JS_OBJECT;
  return JSObject_initialize(&value->u.o);
}

static void JSValue_destroy(JSValue* value) {
  switch (value->kind) {
    case JS_OBJECT: JSObject_destroy(&value->u.o); break;
    case JS_ARRAY: JSArray_destroy(&value->u.a); break;
  }
}


typedef struct {
  const char* start;
  const char* limit;
  const char* p;
  char error[256];
} JSParser;

static inline void JSParser_initialize(JSParser* parser, const char* data, ssize_t length) {
  parser->start = data;
  parser->p = data;
  parser->limit = data + length;
  parser->error[0] = '\0';
}

// Returns false for convenience.
static bool JSParser_error(JSParser* parser, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(parser->error, sizeof(parser->error), fmt, args);
  va_end(args);
  return false;
}

static const char* JSParser_skip_spaces(JSParser* parser) {
  const char* p = parser->p;
  while (isspace(*p)) p++;
  parser->p = p;
  return p;
}

static char JSParser_next(JSParser* parser) {
  if (parser->p < parser->limit)
    return *parser->p++;
  return '\0';
}

static bool JSParser_expect(JSParser* parser, const char* s) {
  ssize_t length = strlen(s) - 1; // First character has already matched
  if (!memcmp(parser->p, s + 1, length)) { // Parser text buffer is zero-terminated
    parser->p += length;
    return true;
  }
  return JSParser_error(parser, "Invalid JSON value (%s?)", s);
}

static bool JSParser_at_end(JSParser* parser) {
  return JSParser_skip_spaces(parser) == parser->limit;
}

static bool is_numeric(char c) {
  return isdigit(c) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E';
}

static bool JSParser_parse_value(JSParser* parser, JSValue* value) {
  if (JSParser_at_end(parser))
    return JSParser_error(parser, "Unexpected end-of-file");

  const char c = JSParser_next(parser);
  switch (c) {
    case 'n': // null
      return JSParser_expect(parser, "null");
    case 't': // true
      JSValue_make_boolean(value, true);
      return JSParser_expect(parser, "true");
    case 'f': // false
      JSValue_make_boolean(value, false);
      return JSParser_expect(parser, "false");
    case '"': { // string
      JSValue_make_string(value, parser->p);
      // Zero-terminated string is always shorter than its external representation
      // therefore it can be created in the parser buffer.
      char* p = (char*) parser->p;
      for (unsigned char c; (c = JSParser_next(parser)) != '"'; *p++ = c) {
        if (c < ' ')
          return JSParser_error(parser, parser->p == parser->limit ? "Unterminated string" : "Unescaped control character in string");
        if (c != '\\') continue;
        c = JSParser_next(parser);
        switch (c) {
          case '"':
          case '\\':
          case '/': continue;
          case 'b': c = '\b'; continue;
          case 'f': c = '\f'; continue;
          case 'n': c = '\n'; continue;
          case 'r': c = '\r'; continue;
          case 't': c = '\t'; continue;
        }
        return JSParser_error(parser, "Hex and unicode escape sequences are not yet supported");
      }
      *p = '\0';
      return true;
    }
    case '{': // object
      for (JSObject* object = JSValue_make_object(value); *JSParser_skip_spaces(parser) != '}';) {
        if (object->fields && *parser->p++ != ',')
          return JSParser_error(parser, "Expected , or } after object property");
        if (*JSParser_skip_spaces(parser) != '"')
          return JSParser_error(parser, "Expected object key");
        parser->p++; // Skip '"'
        const char* name = parser->p;
        for (char c; (c = JSParser_next(parser)) != '"';)
          if (!c) return JSParser_error(parser, "Unterminated key");
        *(char*)(parser->p - 1) = '\0';
        if (*JSParser_skip_spaces(parser) != ':')
          return JSParser_error(parser, "Expected : after object key");
        parser->p++; // Skip ':'
        JSParser_parse_value(parser, &JSField_create(object, name)->value);
      }
      parser->p++; // Skip '}'
      return true;
    case '[': // array
      for (JSArray* array = JSValue_make_array(value); *JSParser_skip_spaces(parser) != ']';) {
        if (array->length && *parser->p++ != ',')
          return JSParser_error(parser, "Expected , or ] after array element");
        JSParser_parse_value(parser, JSArray_append(array));
      }
      parser->p++; // Skip ']'
      return true;
    default:
      if (is_numeric(c)) {
        const char* start = parser->p - 1;
        const char* p = start;
        while (is_numeric(*++p)); // Skip the number
        parser->p = p;

        char* end;
        int64_t i = strtoll(start, &end, 10);
        // On overflow strtoll caps the result by INT64_MIN and INT64_MAX
        if (end == p && i > INT64_MIN && i < INT64_MAX) {
          JSValue_make_integer(value, i);
          return true;
        }
        double d = strtod(start, &end);
        if (end == p) {
          JSValue_make_double(value, d);
          return true;
        }
      }
      return JSParser_error(parser, "Invalid JSON value");
  }
  return false;
}

typedef struct {
  ssize_t length;
  ssize_t capacity;
  char* data;
  int indent;
  uint64_t nexts;  // Stack of bit nexts of nested lists
} JSBuilder;
static inline void JSBuilder_initialize(JSBuilder* builder) {
  builder->indent = 0;
  builder->nexts = 0;
  builder->length = 0;
  builder->capacity = 16*1024; // Initial buffer size
  builder->data = malloc(builder->capacity);
  //TODO: handle possible OOME
}
static inline void JSBuilder_destroy(JSBuilder* builder) {
  free(builder->data);
}
static inline void JSBuilder_send_and_destroy(JSBuilder* builder) {
  write_packet(builder->data, builder->length);
  JSBuilder_destroy(builder);
}

// Bit stack manipulations.
static inline uint64_t JSBuilder_set_next(JSBuilder* builder) {
  const uint64_t nexts = builder->nexts;
  builder->nexts |= 1;
  return nexts & 1;
}
static inline void JSBuilder_push_nexts(JSBuilder* builder) {
  assert(((int64_t)builder->nexts) >= 0);
  builder->nexts <<= 1;
}
static inline void JSBuilder_pop_nexts(JSBuilder* builder) {
  builder->nexts >>= 1;
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
static void JSBuilder_append_unsigned(JSBuilder* builder, uint64_t v) {
  char buffer[22];
  snprintf(buffer, sizeof buffer, "%" PRIu64, v);
  JSBuilder_append_string(builder, buffer);
}
static inline void JSBuilder_append_bool(JSBuilder* builder, bool v) {
  JSBuilder_append_string(builder, v ? "true" : "false");
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
      case '\b': *s = 'b'; break;
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
static void JSBuilder_write_quoted_string(JSBuilder* builder, const char* s) {
  JSBuilder_write_quoted_text(builder, s, strlen(s));
}

enum { JSIndentStep = 2 };
static inline void JSBuilder_indent(JSBuilder* builder) {
  builder->indent += JSIndentStep;
  JSBuilder_push_nexts(builder);
}
static inline void JSBuilder_unindent(JSBuilder* builder) {
  builder->indent -= JSIndentStep;
  assert(builder->indent >= 0);
  JSBuilder_pop_nexts(builder);
}
static void JSBuilder_newline(JSBuilder* builder) {
  int chars_to_append = builder->indent + 1;
  char* data = JSBuilder_allocate(builder, chars_to_append);
  *data++ = '\n';
  while (--chars_to_append > 0)
    *data++ = ' ';
}

static inline void JSBuilder_next(JSBuilder* builder) {
  if (JSBuilder_set_next(builder))
    JSBuilder_append_char(builder, ',');
  JSBuilder_newline(builder);
}

static void JSBuilder_write_field(JSBuilder* builder, const char* name) {
  JSBuilder_next(builder);
  JSBuilder_write_quoted_raw_string(builder, name);
  JSBuilder_append_string(builder, ": ");
}
static void JSBuilder_write_raw_string_field(JSBuilder* builder, const char* name, const char* value) {
  JSBuilder_write_field(builder, name);
  JSBuilder_write_quoted_raw_string(builder, value);
}
static void JSBuilder_write_string_field(JSBuilder* builder, const char* name, const char* value) {
  JSBuilder_write_field(builder, name);
  JSBuilder_write_quoted_string(builder, value);
}
static void JSBuilder_write_unsigned_field(JSBuilder* builder, const char* name, uint64_t value) {
  JSBuilder_write_field(builder, name);
  JSBuilder_append_unsigned(builder, value);
}
static void JSBuilder_write_bool_field(JSBuilder* builder, const char* name, bool value) {
  JSBuilder_write_field(builder, name);
  JSBuilder_append_bool(builder, value);
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
static void JSBuilder_object_field_begin(JSBuilder* builder, const char* name) {
  JSBuilder_write_field(builder, name);
  JSBuilder_object_begin(builder);
}
static inline void JSBuilder_object_field_end(JSBuilder* builder) {
  JSBuilder_object_end(builder);
}
static void JSBuilder_array_field_begin(JSBuilder* builder, const char* name) {
  JSBuilder_write_field(builder, name);
  JSBuilder_array_begin(builder);
}
static inline void JSBuilder_array_field_end(JSBuilder* builder) {
  JSBuilder_array_end(builder);
}

static void JSBuilder_send_and_destroy_object(JSBuilder* builder) {
  JSBuilder_object_end(builder);
  JSBuilder_send_and_destroy(builder);
}

static void JSBuilder_write_seq_0(JSBuilder* builder) {
  JSBuilder_write_field(builder, "seq"); JSBuilder_append_char(builder, '0');
}
static void JSBuilder_initialize_simple_event(JSBuilder* builder, const char* name) {
  JSBuilder_initialize(builder);
  JSBuilder_object_begin(builder);
  JSBuilder_write_seq_0(builder);
  JSBuilder_write_raw_string_field(builder, "type", "event");
  JSBuilder_write_raw_string_field(builder, "event", name);
}
static inline void JSBuilder_send_and_destroy_simple_event(JSBuilder* builder) {
  JSBuilder_send_and_destroy_object(builder);
}
static void send_simple_event(const char* name) {
  JSBuilder builder;
  JSBuilder_initialize_simple_event(&builder, name);
  JSBuilder_send_and_destroy_simple_event(&builder);
}

static void JSBuilder_initialize_event(JSBuilder* builder, const char* name) {
  JSBuilder_initialize_simple_event(builder, name);
  JSBuilder_object_field_begin(builder, "body");
}
static void JSBuilder_send_and_destroy_event(JSBuilder* builder) {
  JSBuilder_object_end(builder); // body
  JSBuilder_send_and_destroy_simple_event(builder);
}

// "StoppedEvent": {
//   "allOf": [ { "$ref": "#/definitions/Event" }, {
//     "type": "object",
//     "description": "Event message for 'stopped' event type. The event
//                     indicates that the execution of the debuggee has stopped
//                     due to some condition. This can be caused by a break
//                     point previously set, a stepping action has completed,
//                     by executing a debugger statement etc.",
//     "properties": {
//       "event": {
//         "type": "string",
//         "enum": [ "stopped" ]
//       },
//       "body": {
//         "type": "object",
//         "properties": {
//           "reason": {
//             "type": "string",
//             "description": "The reason for the event. For backward
//                             compatibility this string is shown in the UI if
//                             the 'description' attribute is missing (but it
//                             must not be translated).",
//             "_enum": [ "step", "breakpoint", "exception", "pause", "entry" ]
//           },
//           "description": {
//             "type": "string",
//             "description": "The full reason for the event, e.g. 'Paused
//                             on exception'. This string is shown in the UI
//                             as is."
//           },
//           "threadId": {
//             "type": "integer",
//             "description": "The thread which was stopped."
//           },
//           "text": {
//             "type": "string",
//             "description": "Additional information. E.g. if reason is
//                             'exception', text contains the exception name.
//                             This string is shown in the UI."
//           },
//           "allThreadsStopped": {
//             "type": "boolean",
//             "description": "If allThreadsStopped is true, a debug adapter
//                             can announce that all threads have stopped.
//                             The client should use this information to
//                             enable that all threads can be expanded to
//                             access their stacktraces. If the attribute
//                             is missing or false, only the thread with the
//                             given threadId can be expanded."
//           }
//         },
//         "required": [ "reason" ]
//       }
//     },
//     "required": [ "event", "body" ]
//   }]
// }
typedef enum { // Extendable - please add custom stop reasons as necessary.
  STOP_REASON_STEP,
  STOP_REASON_BREAKPOINT,
  STOP_REASON_EXCEPTION,
  STOP_REASON_PAUSE,
  STOP_REASON_ENTRY
} StopReason;

static inline const char* stop_reason(const StopReason kind) {
  static const char* const names[] = {
    "step",
    "breakpoint",
    "exception",
    "pause",
    "entry"
  };
  return names[kind];
}

static void send_thread_stopped(int64_t thread_id, StopReason reason, const char* description) {
  JSBuilder builder;
  JSBuilder_initialize_event(&builder, "stopped");
  JSBuilder_write_raw_string_field(&builder, "reason", stop_reason(reason));
  if (description)
    JSBuilder_write_raw_string_field(&builder, "description", description);
  JSBuilder_write_unsigned_field(&builder, "threadId", thread_id);
  JSBuilder_write_bool_field(&builder, "allThreadsStopped", true);
  JSBuilder_send_and_destroy_event(&builder);
}
static void send_thread_stopped_at_breakpoint(int64_t thread_id, uint64_t breakpoint_id, uint64_t location_id) {
  char description[64];
  snprintf(description, sizeof description, "breakpoint %" PRIu64 ".%" PRIu64, breakpoint_id, location_id);
  send_thread_stopped(thread_id, STOP_REASON_BREAKPOINT, description);
}

static void send_process_exited(uint64_t exit_code) {
  JSBuilder builder;
  JSBuilder_initialize_event(&builder, "exited");
  JSBuilder_write_unsigned_field(&builder, "exitCode", exit_code);
  JSBuilder_send_and_destroy_event(&builder);
}

static void send_terminated(void) {
  if (!sent_terminated_event) {
    sent_terminated_event = true;
    send_simple_event("terminated");
  }
}

// "ProcessEvent": {
//   "allOf": [
//     { "$ref": "#/definitions/Event" },
//     {
//       "type": "object",
//       "description": "Event message for 'process' event type. The event
//                       indicates that the debugger has begun debugging a
//                       new process. Either one that it has launched, or one
//                       that it has attached to.",
//       "properties": {
//         "event": {
//           "type": "string",
//           "enum": [ "process" ]
//         },
//         "body": {
//           "type": "object",
//           "properties": {
//             "name": {
//               "type": "string",
//               "description": "The logical name of the process. This is
//                               usually the full path to process's executable
//                               file. Example: /home/myproj/program.js."
//             },
//             "systemProcessId": {
//               "type": "integer",
//               "description": "The system process id of the debugged process.
//                               This property will be missing for non-system
//                               processes."
//             },
//             "isLocalProcess": {
//               "type": "boolean",
//               "description": "If true, the process is running on the same
//                               computer as the debug adapter."
//             },
//             "startMethod": {
//               "type": "string",
//               "enum": [ "launch", "attach", "attachForSuspendedLaunch" ],
//               "description": "Describes how the debug engine started
//                               debugging this process.",
//               "enumDescriptions": [
//                 "Process was launched under the debugger.",
//                 "Debugger attached to an existing process.",
//                 "A project launcher component has launched a new process in
//                  a suspended state and then asked the debugger to attach."
//               ]
//             }
//           },
//           "required": [ "name" ]
//         }
//       },
//       "required": [ "event", "body" ]
//     }
//   ]
// }
static inline void send_process_launched(void) {
  JSBuilder builder;
  JSBuilder_initialize_event(&builder, "process");
  JSBuilder_write_string_field(&builder, "name", program_path);
  JSBuilder_write_unsigned_field(&builder, "systemProcessId", program_pid);
  JSBuilder_write_bool_field(&builder, "isLocalProcess", true);
  JSBuilder_write_raw_string_field(&builder, "startMethod", "launch");
  JSBuilder_send_and_destroy_event(&builder);
}

// "Breakpoint": {
//   "type": "object",
//   "description": "Information about a Breakpoint created in setBreakpoints
//                   or setFunctionBreakpoints.",
//   "properties": {
//     "id": {
//       "type": "integer",
//       "description": "An optional unique identifier for the breakpoint."
//     },
//     "verified": {
//       "type": "boolean",
//       "description": "If true breakpoint could be set (but not necessarily
//                       at the desired location)."
//     },
//     "message": {
//       "type": "string",
//       "description": "An optional message about the state of the breakpoint.
//                       This is shown to the user and can be used to explain
//                       why a breakpoint could not be verified."
//     },
//     "source": {
//       "$ref": "#/definitions/Source",
//       "description": "The source where the breakpoint is located."
//     },
//     "line": {
//       "type": "integer",
//       "description": "The start line of the actual range covered by the
//                       breakpoint."
//     },
//     "column": {
//       "type": "integer",
//       "description": "An optional start column of the actual range covered
//                       by the breakpoint."
//     },
//     "endLine": {
//       "type": "integer",
//       "description": "An optional end line of the actual range covered by
//                       the breakpoint."
//     },
//     "endColumn": {
//       "type": "integer",
//       "description": "An optional end column of the actual range covered by
//                       the breakpoint. If no end line is given, then the end
//                       column is assumed to be in the start line."
//     }
//   },
//   "required": [ "verified" ]
// }

// 'verified' means the breakpoint can be set, though its location can be different from the desired.
// If 'file' == null the breakpoint source location is omitted.
// If 'path' == null the source path is omitted.
// If 'line' is zero it is omitted.
static void JSBuilder_write_breakpoint(JSBuilder* builder, int id, bool verified, const char* path, unsigned line, unsigned column) {
  JSBuilder_object_begin(builder);
  JSBuilder_write_unsigned_field(builder, "id", id);
  JSBuilder_write_bool_field(builder, "verified", verified);
  if (path) {
    JSBuilder_write_field(builder, "source");
    JSBuilder_object_begin(builder);
    JSBuilder_write_string_field(builder, "path", path);
    // TODO: In addition to "path" LLDB sets "name" field to basename(path)
    JSBuilder_object_end(builder);
    if (line)
      JSBuilder_write_unsigned_field(builder, "line", line);
    if (column)
      JSBuilder_write_unsigned_field(builder, "column", column);
  }
  JSBuilder_object_end(builder);
}
static void send_breakpoint_changed(int id, bool verified) {
  JSBuilder builder;
  JSBuilder_initialize_event(&builder, "breakpoint");
  JSBuilder_write_field(&builder, "breakpoint");
  // Emit brief breakpoint info without source location
  JSBuilder_write_breakpoint(&builder, id, verified, NULL, 0, 0);
  JSBuilder_write_raw_string_field(&builder, "reason", "changed");
  JSBuilder_send_and_destroy_event(&builder);
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
  JSBuilder_write_field(&builder, "output");
  JSBuilder_write_quoted_text(&builder, data, length);
  JSBuilder_write_raw_string_field(&builder, "category", output_category(out));
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
    snprintf(error_buffer, sizeof error_buffer, "Couldn't create new pipe for fd %d. %s", fd, current_error());
    return error_buffer;
  }
  if (dup2(new_fd[1], fd) == -1) {
    snprintf(error_buffer, sizeof error_buffer, "Couldn't override the fd %d. %s", fd, current_error());
    return error_buffer;
  }

  int args[2] = {new_fd[0], (int)out};
  pthread_t thread;
  if (pthread_create(&thread, NULL, &redirect_output_loop, &args)) {
    snprintf(error_buffer, sizeof error_buffer, "Couldn't create the redirect thread for fd %d. %s", fd, current_error());
    return error_buffer;
  }
  return NULL;
}

static void redirect_output(FILE* file, const OutputType out) {
  const char* error = redirect_fd(fileno(file), out);
  if (error) {
    log_printf("%s\n", error);
    send_output(STDERR, error, strlen(error));
  }
}

static inline void launch_target_in_terminal(const char* comm_file, const int argc, char* const argv[]) {
  fprintf(stderr, "launch_target_in_terminal is not implemented\n");
  exit(EXIT_FAILURE);
}

typedef struct DOUBLY_LINKED {
  struct DOUBLY_LINKED* next;
  struct DOUBLY_LINKED* prev;
} DoublyLinked;

typedef struct DELAYED_REQUEST {
  DoublyLinked link;
  const char* command;  // Not owned by Request
  uint64_t request_seq;
  void (*handle) (struct DELAYED_REQUEST* req);
} DelayedRequest;

static const char* canonicalize_request_name(const char* name);
static void* DelayedRequest_create(const JSObject* request, size_t size, void (*handle)(DelayedRequest*)) {
  DelayedRequest* req = malloc(size);
  req->command = canonicalize_request_name(JSObject_get_string_field(request, "command"));
  req->request_seq = JSObject_get_integer_field(request, "seq", 0);
  req->handle = handle;
  return req;
}

static DoublyLinked request_queue = {&request_queue, &request_queue};
static pthread_mutex_t request_queue_lock;
static inline bool request_queue_not_empty(void) {
  return request_queue.next != &request_queue;
}
static inline void insert_to_request_queue(DelayedRequest* req) {
  if (sent_terminated_event) return;

  pthread_mutex_lock(&request_queue_lock);

  DoublyLinked* it = &req->link;
  DoublyLinked* prev = request_queue.prev;
  request_queue.prev = it; it->prev = prev;
  prev->next = it; it->next = &request_queue;

  pthread_mutex_unlock(&request_queue_lock);
}
static inline DelayedRequest* remove_from_request_queue(void) {
  assert(request_queue_not_empty());
  pthread_mutex_lock(&request_queue_lock);

  DoublyLinked* it = request_queue.next;
  DoublyLinked* next = it->next;
  request_queue.next = next;
  next->prev = &request_queue;

  pthread_mutex_unlock(&request_queue_lock);
  return (DelayedRequest*) it;
}

// TODO: call handle_pending_debug_requests() from Stanza debugger
static void handle_pending_debug_requests(void) {
  while (request_queue_not_empty()) {
    DelayedRequest* req = remove_from_request_queue();
    req->handle(req);
    free(req);
  }
}

static inline unsigned request_queue_length(void) {
  int count = 0;
  for (const DoublyLinked* it = request_queue.next; it != &request_queue; it = it->next)
    ++count;
  return count;
}
// This function is not thread-safe. It is only intended for debuggung.
static void print_request_queue(void) {
  log_printf("Request queue (length: %u):\n", request_queue_length());
  for (const DoublyLinked* it = request_queue.next; it != &request_queue; it = it->next) {
    const DelayedRequest* req = (const DelayedRequest*) it;
    log_printf("  %s\n", req->command);
  }
}

static void JSBuilder_initialize_response_to_command(JSBuilder* builder, const char* command, uint64_t request_seq, const char* message) {
  JSBuilder_initialize(builder);
  JSBuilder_object_begin(builder);
  JSBuilder_write_seq_0(builder);
  JSBuilder_write_raw_string_field(builder, "type", "response");
  JSBuilder_write_raw_string_field(builder, "command", command);
  JSBuilder_write_unsigned_field(builder, "request_seq", request_seq);
  JSBuilder_write_bool_field(builder, "success", message == NULL);
  if (message)
    JSBuilder_write_string_field(builder, "message", message);
}
static void JSBuilder_initialize_response(JSBuilder* builder, const JSObject* request, const char* message) {
  const char* command = JSObject_get_string_field(request, "command");
  const uint64_t request_seq = JSObject_get_integer_field(request, "seq", 0);
  JSBuilder_initialize_response_to_command(builder, command, request_seq, message);
}
static void JSBuilder_initialize_delayed_response(JSBuilder* builder, const DelayedRequest* request, const char* message) {
  JSBuilder_initialize_response_to_command(builder, request->command, request->request_seq, message);
}
static inline void JSBuilder_send_and_destroy_response(JSBuilder* builder) {
  JSBuilder_send_and_destroy_object(builder);
}
static void respond_to_request(const JSObject* request, const char* message) {
  JSBuilder builder;
  JSBuilder_initialize_response(&builder, request, message);
  JSBuilder_send_and_destroy_response(&builder);
}
static void respond_to_delayed_request(const DelayedRequest* request, const char* message) {
  JSBuilder builder;
  JSBuilder_initialize_delayed_response(&builder, request, message);
  JSBuilder_send_and_destroy_response(&builder);
}

// "InitializeRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "Initialize request; value of command field is
//                     'initialize'.",
//     "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "initialize" ]
//       },
//       "arguments": {
//         "$ref": "#/definitions/InitializeRequestArguments"
//       }
//     },
//     "required": [ "command", "arguments" ]
//   }]
// },
// "InitializeRequestArguments": {
//   "type": "object",
//   "description": "Arguments for 'initialize' request.",
//   "properties": {
//     "clientID": {
//       "type": "string",
//       "description": "The ID of the (frontend) client using this adapter."
//     },
//     "adapterID": {
//       "type": "string",
//       "description": "The ID of the debug adapter."
//     },
//     "locale": {
//       "type": "string",
//       "description": "The ISO-639 locale of the (frontend) client using
//                       this adapter, e.g. en-US or de-CH."
//     },
//     "linesStartAt1": {
//       "type": "boolean",
//       "description": "If true all line numbers are 1-based (default)."
//     },
//     "columnsStartAt1": {
//       "type": "boolean",
//       "description": "If true all column numbers are 1-based (default)."
//     },
//     "pathFormat": {
//       "type": "string",
//       "_enum": [ "path", "uri" ],
//       "description": "Determines in what format paths are specified. The
//                       default is 'path', which is the native format."
//     },
//     "supportsVariableType": {
//       "type": "boolean",
//       "description": "Client supports the optional type attribute for
//                       variables."
//     },
//     "supportsVariablePaging": {
//       "type": "boolean",
//       "description": "Client supports the paging of variables."
//     },
//     "supportsRunInTerminalRequest": {
//       "type": "boolean",
//       "description": "Client supports the runInTerminal request."
//     }
//   },
//   "required": [ "adapterID" ]
// },
// "InitializeResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'initialize' request.",
//     "properties": {
//       "body": {
//         "$ref": "#/definitions/Capabilities",
//         "description": "The capabilities of this debug adapter."
//       }
//     }
//   }]
// }
static inline void define_capabilities(JSBuilder* builder) {
  typedef struct { const char* name; bool value; } capability;
  static const capability capabilities[] = {
    {"supportsConfigurationDoneRequest", false},
    {"supportsFunctionBreakpoints", false},
    {"supportsConditionalBreakpoints", false},
    // TODO: Supports breakpoints that break execution after a specified number of hits.
    {"supportsHitConditionalBreakpoints", false},
    // TODO: Supports a (side effect free) evaluate request for data hovers.
    {"supportsEvaluateForHovers", false},
    // TODO: Supports launching a debugee in intergrated VSCode terminal.
    {"supportsRunInTerminalRequest", false},
    // TODO: Supports stepping back via the stepBack and reverseContinue requests.
    {"supportsStepBack", false},
    // TODO: The debug adapter supports setting a variable to a value.
    {"supportsSetVariable", false},
    {"supportsRestartFrame", false},
    {"supportsGotoTargetsRequest", false},
    {"supportsStepInTargetsRequest", false},
    // See the note on inherent inefficiency of completions in LLDB implementation.
    {"supportsCompletionsRequest", false},
    {"supportsModulesRequest", false},
    // The set of additional module information exposed by the debug adapter.
    //   body.try_emplace("additionalModuleColumns"] = ColumnDescriptor
    // Checksum algorithms supported by the debug adapter.
    //   body.try_emplace("supportedChecksumAlgorithms"] = ChecksumAlgorithm
    // The debugger does not support the RestartRequest. The client must implement
    // 'restart' by terminating and relaunching the adapter.
    {"supportsRestartRequest", false},
    // TODO: support 'exceptionOptions' on the setExceptionBreakpoints request.
    {"supportsExceptionOptions", false},
    // TODO: Support a 'format' attribute on the stackTraceRequest, variablesRequest, and evaluateRequest.
    {"supportsValueFormattingOptions", false},
    // TODO: support the exceptionInfo request.
    {"supportsExceptionInfoRequest", false},
    // TODO: support the 'terminateDebuggee' attribute on the 'disconnect' request.
    {"supportTerminateDebuggee", false},
    // The debugger does not support the delayed loading of parts of the stack,
    // which would require support for 'startFrame' and 'levels' arguments and the
    // 'totalFrames' result of the 'StackTrace' request.
    {"supportsDelayedStackTraceLoading", false},
    // TODO: support the 'loadedSources' request.
    {"supportsLoadedSourcesRequest", false},
    // Do we need this?
    {"supportsProgressReporting", false},
    {NULL, false} // Convenient NULL termunation.
  };
  for (const capability* p = capabilities; p->name; p++)
    JSBuilder_write_bool_field(builder, p->name, p->value);

  #if 0
    // TODO:
    // Available filters or options for the setExceptionBreakpoints request.
    llvm::json::Array filters;
    for (const auto &exc_bp : g_vsc.exception_breakpoints) {
      filters.emplace_back(CreateExceptionBreakpointFilter(exc_bp));
    }
    body.try_emplace("exceptionBreakpointFilters", std::move(filters));
  #endif
}
static bool request_initialize(const JSObject* request) {
  // TODO: initialize debugger here.
  JSBuilder builder;
  JSBuilder_initialize_response(&builder, request, NULL);
  JSBuilder_object_field_begin(&builder, "body");
  {
    define_capabilities(&builder);
  }
  JSBuilder_object_field_end(&builder); // body
  JSBuilder_send_and_destroy_response(&builder);
  return true;
}

// "LaunchRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "Launch request; value of command field is 'launch'.",
//     "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "launch" ]
//       },
//       "arguments": {
//         "$ref": "#/definitions/LaunchRequestArguments"
//       }
//     },
//     "required": [ "command", "arguments"  ]
//   }]
// },
// "LaunchRequestArguments": {
//   "type": "object",
//   "description": "Arguments for 'launch' request.",
//   "properties": {
//     "noDebug": {
//       "type": "boolean",
//       "description": "If noDebug is true the launch request should launch
//                       the program without enabling debugging."
//     }
//   }
// },
// "LaunchResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'launch' request. This is just an
//                     acknowledgement, so no body field is required."
//   }]
// }
static const char* get_string_array(const JSObject* object, const char* name, int reserve_at_start, const char*** out) {
  const JSField* field = JSObject_get_field(object, name, JS_ARRAY);
  const int length = field ? field->value.u.a.length : 0;
  const char** q = (const char**) malloc((length + reserve_at_start + 1) * sizeof(char*));
  *out = q;
  q += reserve_at_start;
  if (length) {
    for (const JSValue *p = field->value.u.a.data, *limit = p + length; p < limit; p++) {
      if (p->kind != JS_STRING) {
        static char message[64];
        // TODO: Print detailed message here with the index and value. maybe try to convert the value to string first.
        // It looks like we only need JS arrays of strings. JSParser can be modified to store a pointer to source representation in a field.
        snprintf(message, sizeof message, "%s: array of strings expected", name);
        return message;
      }
      *q++ = p->u.s;
    }
  }
  *q = NULL;
  return NULL;
}

// Skeleton of Stanza debgger. It runs on a separate thread.
// TODO: replace it woth LoStanza function.
static void* handle_launch(void* args) {
  char** argv = (char**) args;
  // TODO: Replace the code below with the debugger run with 'argv'
  log_printf("! Running stanza program %s with arguments:\n", argv[0]);
  for (char* s; (s = *++argv) != NULL;)
    log_printf("  %s\n", s);

  send_thread_stopped(1234567, STOP_REASON_ENTRY, "Stopped at entry");

  for (int i = 0; i < 1000; i++) {
    printf("Program iteration %d\n", i);
    handle_pending_debug_requests();
  }
  send_process_exited(0);
  return NULL;
}

static inline const char* launch_program(const JSObject* request_arguments) {
  const char* cwd = JSObject_get_string_field(request_arguments, "cwd");
  if (cwd && chdir(cwd)) return current_error();

  const bool stop_at_entry = JSObject_get_bool_field(request_arguments, "stopOnEntry", false);
  // O.P.: Do we need this?
  // const bool runInTerminal = JSObject_get_boolean_field(request_arguments, "runInTerminal", false);

  const char* program = JSObject_get_string_field(request_arguments, "program");
  if (!program) return "no program specified";

  const char** args = NULL;
  const char* error = get_string_array(request_arguments, "args", 1, &args);
  if (!error) {
    const char** env = NULL;
    error = get_string_array(request_arguments, "env", 0, &env);
    if (!error) {
      free_path(program_path);
      program_path = get_absolute_path(program);
      args[0] = program_path;
      // TODO: launch the program here, remember program_pid
      // For now, lets just run stanza debugger in-process on a separate thread and ignore 'env'
      pthread_t program_thread;
      if (pthread_create(&program_thread, NULL, &handle_launch, args)) {
        static char error_buffer[256];
        snprintf(error_buffer, sizeof error_buffer, "Couldn't create stanza program thread. %s", current_error());
        error = error_buffer;
      }
      program_pid = getpid();
    }
    free(env);
  }
  // Yes, 'args' leaks memory as it can be used by program_thread. Please uncomment the line below when a program launched as a process.
  // free(args);
  return error;
}
static bool request_launch(const JSObject* request) {
  const JSObject* arguments = JSObject_get_object_field(request, "arguments");
  //TODO: Do we really need built-in debugger commands interpreter?
  // init_commands = GetStrings(arguments, "initCommands");
  // pre_run_commands = GetStrings(arguments, "preRunCommands");
  // stop_commands = GetStrings(arguments, "stopCommands");
  // exit_commands = GetStrings(arguments, "exitCommands");
  // terminate_commands = GetStrings(arguments, "terminateCommands");
  // auto launchCommands = GetStrings(arguments, "launchCommands");
  // std::vector<std::string> postRunCommands = GetStrings(arguments, "postRunCommands");

  // TODO: Do we really need debuggerRoot?
  // const char* debugger_root = JSObject_get_string_field(arguments, "debuggerRoot");
  // const uint64_t timeout_seconds = JSObject_get_integer_field(arguments, "timeout", 30);

  // This is a hack for loading DWARF in .o files on Mac where the .o files
  // in the debug map of the main executable have relative paths which require
  // the lldb-vscode binary to have its working directory set to that relative
  // root for the .o files in order to be able to load debug info.
  // if (!debuggerRoot.empty())
  //  llvm::sys::fs::set_current_path(debuggerRoot);

  // Run any initialize LLDB commands the user specified in the launch.json.
  // This is run before target is created, so commands can't do anything with
  // the targets - preRunCommands are run with the target.
  // RunInitCommands();

  //TODO: Do we really need sourcePath or sourceMap?
  // LLDB supports only sourceMap and ignores sourcePath.
  // SetSourceMapFromArguments(*arguments);
  const char* error = launch_program(arguments);
  respond_to_request(request, error);
  if (error)
    log_printf("launch_request error: %s\n", error);
  else
    send_process_launched();
  send_simple_event("initialized");
  return true;
}

// VSCode issues separate setBreakpoints request for each source file
// where some breakpoints are currently set or were set before.
// The request lists all breakpoints in this file.
// The debugger has to sync its breakpoints for this file with breakpoints in this list.
//
// "SetBreakpointsRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "SetBreakpoints request; value of command field is
//     'setBreakpoints'. Sets multiple breakpoints for a single source and
//     clears all previous breakpoints in that source. To clear all breakpoint
//     for a source, specify an empty array. When a breakpoint is hit, a
//     StoppedEvent (event type 'breakpoint') is generated.", "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "setBreakpoints" ]
//       },
//       "arguments": {
//         "$ref": "#/definitions/SetBreakpointsArguments"
//       }
//     },
//     "required": [ "command", "arguments"  ]
//   }]
// },
// "SetBreakpointsArguments": {
//   "type": "object",
//   "description": "Arguments for 'setBreakpoints' request.",
//   "properties": {
//     "source": {
//       "$ref": "#/definitions/Source",
//       "description": "The source location of the breakpoints; either
//       source.path or source.reference must be specified."
//     },
//     "breakpoints": {
//       "type": "array",
//       "items": {
//         "$ref": "#/definitions/SourceBreakpoint"
//       },
//       "description": "The code locations of the breakpoints."
//     },
//     "lines": {
//       "type": "array",
//       "items": {
//         "type": "integer"
//       },
//       "description": "Deprecated: The code locations of the breakpoints."
//     },
//     "sourceModified": {
//       "type": "boolean",
//       "description": "A value of true indicates that the underlying source
//       has been modified which results in new breakpoint locations."
//     }
//   },
//   "required": [ "source" ]
// },
// "SetBreakpointsResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'setBreakpoints' request. Returned is
//     information about each breakpoint created by this request. This includes
//     the actual code location and whether the breakpoint could be verified.
//     The breakpoints returned are in the same order as the elements of the
//     'breakpoints' (or the deprecated 'lines') in the
//     SetBreakpointsArguments.", "properties": {
//       "body": {
//         "type": "object",
//         "properties": {
//           "breakpoints": {
//             "type": "array",
//             "items": {
//               "$ref": "#/definitions/Breakpoint"
//             },
//             "description": "Information about the breakpoints. The array
//             elements are in the same order as the elements of the
//             'breakpoints' (or the deprecated 'lines') in the
//             SetBreakpointsArguments."
//           }
//         },
//         "required": [ "breakpoints" ]
//       }
//     },
//     "required": [ "body" ]
//   }]
// },
// "SourceBreakpoint": {
//   "type": "object",
//   "description": "Properties of a breakpoint or logpoint passed to the
//   setBreakpoints request.", "properties": {
//     "line": {
//       "type": "integer",
//       "description": "The source line of the breakpoint or logpoint."
//     },
//     "column": {
//       "type": "integer",
//       "description": "An optional source column of the breakpoint."
//     },
//     "condition": {
//       "type": "string",
//       "description": "An optional expression for conditional breakpoints."
//     },
//     "hitCondition": {
//       "type": "string",
//       "description": "An optional expression that controls how many hits of
//       the breakpoint are ignored. The backend is expected to interpret the
//       expression as needed."
//     },
//     "logMessage": {
//       "type": "string",
//       "description": "If this attribute exists and is non-empty, the backend
//       must not 'break' (stop) but log the message instead. Expressions within
//       {} are interpolated."
//     }
//   },
//   "required": [ "line" ]
// }

// File path is specified separately.
typedef struct {
  unsigned line;    // 1-based
  unsigned column;  // 1-based, 0 denotes undefined column
} SourceBreakpoint;

typedef struct {
  ssize_t length;
  ssize_t capacity;
  SourceBreakpoint* data;
} SourceBreakpointVector;
static void SourceBreakpointVector_initialize(SourceBreakpointVector* vector) {
  vector->length = 0;
  vector->capacity = 16; // Initial vector size
  vector->data = malloc(vector->capacity * sizeof(SourceBreakpoint));
  //TODO: handle possible OOME
}
static inline void SourceBreakpointVector_destroy(SourceBreakpointVector* vector) {
  free(vector->data);
}
static SourceBreakpoint* SourceBreakpointVector_allocate(SourceBreakpointVector* vector) {
  if (vector->length == vector->capacity) {
    vector->capacity <<= 1;
    vector->data = realloc(vector->data, vector->capacity * sizeof(SourceBreakpoint));
    //TODO: handle possible OOM
  }
  return vector->data + vector->length++;
}

typedef struct {
  uint32_t id;
  unsigned line;    // 1-based
  unsigned column;  // 1-based, 0 denotes undefined column
  bool verified;
} Breakpoint;

typedef struct {
  ssize_t length;
  ssize_t capacity;
  Breakpoint* data;
} BreakpointVector;
static void BreakpointVector_initialize(BreakpointVector* vector) {
  vector->length = 0;
  vector->capacity = 16; // Initial vector size
  vector->data = malloc(vector->capacity * sizeof(Breakpoint));
  //TODO: handle possible OOME
}
static inline void BreakpointVector_destroy(BreakpointVector* vector) {
  free(vector->data);
}
static Breakpoint* BreakpointVector_allocate(BreakpointVector* vector) {
  if (vector->length == vector->capacity) {
    vector->capacity <<= 1;
    vector->data = realloc(vector->data, vector->capacity * sizeof(Breakpoint));
    //TODO: handle possible OOM
  }
  return vector->data + vector->length++;
}

static bool request_setBreakpoints(const JSObject* request) {
  const JSObject* arguments = JSObject_get_object_field(request, "arguments");
  const JSObject* source = JSObject_get_object_field(arguments, "source");
  const char* path = JSObject_get_string_field(source, "path");
  const JSArray* breakpoints = JSObject_get_array_field(arguments, "breakpoints");

  SourceBreakpointVector in_breakpoints;
  SourceBreakpointVector_initialize(&in_breakpoints);

  BreakpointVector out_breakpoints;
  BreakpointVector_initialize(&out_breakpoints);

  if (path) {
    if (breakpoints) {
      // Validate brakpoints and store in in_breakpoints.
      for (const JSValue *p = breakpoints->data, *const limit = p + breakpoints->length; p < limit; p++) {
        if (p->kind == JS_OBJECT) {
          const JSObject* o = &p->u.o;
          const int64_t line = JSObject_get_integer_field(o, "line", 0);
          const int64_t column = JSObject_get_integer_field(o, "column", 0);
          // TODO: get optional condition, hitCondition and logMessage fields.
          if (line <= 0 || column < 0) continue;
          SourceBreakpoint* sbp = SourceBreakpointVector_allocate(&in_breakpoints);
          sbp->line = line;
          sbp->column = column;
        }
      }
    }
    // TODO: Pass in_breakponts (in_breakponts.data, in_breakponts.length), path and &out_breakponts to the debugger core here.
    // Copy in to out just to fake the effect
    for (int id = 0, length = in_breakpoints.length; id < length; id++) {
      const SourceBreakpoint* sbp = in_breakpoints.data + id;
      Breakpoint* bp = BreakpointVector_allocate(&out_breakpoints);
      bp->id = id;
      bp->line = sbp->line;
      bp->column = sbp->column;
      bp->verified = false;
    }
  }

  JSBuilder builder;
  JSBuilder_initialize_response(&builder, request, NULL);
  if (path) {
    JSBuilder_object_field_begin(&builder, "body");
    {
      JSBuilder_array_field_begin(&builder, "breakpoints");
      {
        // TODO: Replace in_breakpoints with out_breakpoints here.
        for (const Breakpoint *p = out_breakpoints.data, *const limit = p + out_breakpoints.length; p < limit; p++) {
          JSBuilder_next(&builder);
          JSBuilder_write_breakpoint(&builder, p->id, p->verified, path, p->line, p->column);
        }
      }
      JSBuilder_array_field_end(&builder); // breakpoints
    }
    JSBuilder_object_field_end(&builder); // body
  }
  JSBuilder_send_and_destroy_response(&builder);

  SourceBreakpointVector_destroy(&in_breakpoints);
  return true;
}

// "SetExceptionBreakpointsRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "SetExceptionBreakpoints request; value of command field
//     is 'setExceptionBreakpoints'. The request configures the debuggers
//     response to thrown exceptions. If an exception is configured to break, a
//     StoppedEvent is fired (event type 'exception').", "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "setExceptionBreakpoints" ]
//       },
//       "arguments": {
//         "$ref": "#/definitions/SetExceptionBreakpointsArguments"
//       }
//     },
//     "required": [ "command", "arguments"  ]
//   }]
// },
// "SetExceptionBreakpointsArguments": {
//   "type": "object",
//   "description": "Arguments for 'setExceptionBreakpoints' request.",
//   "properties": {
//     "filters": {
//       "type": "array",
//       "items": {
//         "type": "string"
//       },
//       "description": "IDs of checked exception options. The set of IDs is
//       returned via the 'exceptionBreakpointFilters' capability."
//     },
//     "exceptionOptions": {
//       "type": "array",
//       "items": {
//         "$ref": "#/definitions/ExceptionOptions"
//       },
//       "description": "Configuration options for selected exceptions."
//     }
//   },
//   "required": [ "filters" ]
// },
// "SetExceptionBreakpointsResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'setExceptionBreakpoints' request. This is
//     just an acknowledgement, so no body field is required."
//   }]
// }
static bool request_setExceptionBreakpoints(const JSObject* request) {
  const JSObject* arguments = JSObject_get_object_field(request, "arguments");
  const JSArray* filters = JSObject_get_array_field(arguments, "filters");
  // TODO: Cleanup all exception breakpoints in the debugger here.
  // TODO: Set each exception breakpoint in the debugger:
  // for (const JSValue *p = filters->data, *const limit = p + filters->length; p < limit; p++)
  //  if (p->kind == JS_STRING)
  //    TODO: set exception breakpoint in the debugger with p->u.s filter.

  respond_to_request(request, NULL);
  return true;
}

// "ThreadsRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "Thread request; value of command field is 'threads'. The
//     request retrieves a list of all threads.", "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "threads" ]
//       }
//     },
//     "required": [ "command" ]
//   }]
// },
// "ThreadsResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'threads' request.",
//     "properties": {
//       "body": {
//         "type": "object",
//         "properties": {
//           "threads": {
//             "type": "array",
//             "items": {
//               "$ref": "#/definitions/Thread"
//             },
//             "description": "All threads."
//           }
//         },
//         "required": [ "threads" ]
//       }
//     },
//     "required": [ "body" ]
//   }]
// }
static bool request_threads(const JSObject* request) {
  // TODO: Maybe report coroutines instead of threads?
  JSBuilder builder;
  // TODO: If no active stack exists, pass an error message instead of NULL here.
  JSBuilder_initialize_response(&builder, request, NULL);
  JSBuilder_object_field_begin(&builder, "body");
  {
    // TODO: Should we list coroutines here?
    JSBuilder_array_field_begin(&builder, "threads");
    {
      JSBuilder_next(&builder);
      JSBuilder_object_begin(&builder);
      {
        JSBuilder_write_unsigned_field(&builder, "id", 12345678);
        JSBuilder_write_raw_string_field(&builder, "name", "main");
      }
      JSBuilder_object_end(&builder); // an individual thread
    }
    JSBuilder_array_field_end(&builder); // threads
  }
  JSBuilder_object_end(&builder); // body
  JSBuilder_send_and_destroy_response(&builder);
  return true;
}

// "StackTraceRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "StackTrace request; value of command field is
//     'stackTrace'. The request returns a stacktrace from the current execution
//     state.", "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "stackTrace" ]
//       },
//       "arguments": {
//         "$ref": "#/definitions/StackTraceArguments"
//       }
//     },
//     "required": [ "command", "arguments"  ]
//   }]
// },
// "StackTraceArguments": {
//   "type": "object",
//   "description": "Arguments for 'stackTrace' request.",
//   "properties": {
//     "threadId": {
//       "type": "integer",
//       "description": "Retrieve the stacktrace for this thread."
//     },
//     "startFrame": {
//       "type": "integer",
//       "description": "The index of the first frame to return; if omitted
//       frames start at 0."
//     },
//     "levels": {
//       "type": "integer",
//       "description": "The maximum number of frames to return. If levels is
//       not specified or 0, all frames are returned."
//     },
//     "format": {
//       "$ref": "#/definitions/StackFrameFormat",
//       "description": "Specifies details on how to format the stack frames."
//     }
//  },
//   "required": [ "threadId" ]
// },
// "StackTraceResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'stackTrace' request.",
//     "properties": {
//       "body": {
//         "type": "object",
//         "properties": {
//           "stackFrames": {
//             "type": "array",
//             "items": {
//               "$ref": "#/definitions/StackFrame"
//             },
//             "description": "The frames of the stackframe. If the array has
//             length zero, there are no stackframes available. This means that
//             there is no location information available."
//           },
//           "totalFrames": {
//             "type": "integer",
//             "description": "The total number of frames available."
//           }
//         },
//         "required": [ "stackFrames" ]
//       }
//     },
//     "required": [ "body" ]
//   }]
// }
enum { INVALID_THREAD_ID = -1 };

typedef struct {
  uint64_t id;  // Unique id that combines thread/coroutine id and frame id in the stack (i.e. offset from the bottom).
  const char* source_path;    // Referenced here, owned by debugger
  const char* function_name;  // Referenced here, owned by debugger
  int64_t line;   // 1-based
  int64_t column; // 1-based, 0 denotes unknown
} StackTraceFrame;
typedef struct {
  ssize_t length;
  ssize_t capacity;
  StackTraceFrame* data;
} StackTrace;
static void StackTrace_initialize(StackTrace* st) {
  st->length = 0;
  st->capacity = 16; // Initial stack trace size
  st->data = malloc(st->capacity * sizeof(StackTraceFrame));
  //TODO: handle possible OOME
}
static inline void StackTrace_destroy(StackTrace* st) {
  free(st->data);
}
static StackTraceFrame* StackTrace_allocate(StackTrace* st) {
  if (st->length == st->capacity) {
    st->capacity <<= 1;
    st->data = realloc(st->data, st->capacity * sizeof(StackTraceFrame));
    //TODO: handle possible OOM
  }
  return st->data + st->length++;
}

// Create stack trace for given thread_id starting at start level with no more than max_frames frames.
// Frames not visible to the debugger can be skipped.
// Returns total number of frames in thread_id stack or -1 when no thread with given therad_id thread_is found.
static int64_t create_stack_trace(StackTrace* st, int64_t thread_id, int64_t start, int64_t max_frames) {
  // TODO: Implement this function in debugger core.
  return 0;
}

typedef struct {
  DelayedRequest parent;
  int64_t thread_id;
  int64_t start_frame;
  int64_t max_frames;
} DelayedRequestStackTrace;
static void DelayedRequestStackTrace_handle(DelayedRequest* request) {
  const DelayedRequestStackTrace* req = (const DelayedRequestStackTrace*)request;
  StackTrace st;
  StackTrace_initialize(&st);
  const int64_t total_frames = create_stack_trace(&st, req->thread_id, req->start_frame, req->max_frames);

  JSBuilder builder;
  JSBuilder_initialize_delayed_response(&builder, request, NULL);
  JSBuilder_object_field_begin(&builder, "body");
  if (total_frames >= 0) {
    JSBuilder_write_unsigned_field(&builder, "totalFrames", total_frames);
    JSBuilder_array_field_begin(&builder, "stackFrames");
    for (const StackTraceFrame *p = st.data, *const limit = p + st.length; p < limit; p++) {
      JSBuilder_next(&builder);
      JSBuilder_object_begin(&builder);
      {
        JSBuilder_write_unsigned_field(&builder, "id", p->id);
        JSBuilder_write_string_field(&builder, "name", p->function_name);
        JSBuilder_write_string_field(&builder, "source", p->source_path);
        JSBuilder_write_unsigned_field(&builder, "line", p->line);
        if (p->column)
          JSBuilder_write_unsigned_field(&builder, "column", p->column);
      }
      JSBuilder_object_end(&builder);
    }
    JSBuilder_array_field_end(&builder); // stackFrames
  }
  JSBuilder_object_field_end(&builder); // body
  JSBuilder_send_and_destroy_response(&builder);

  StackTrace_destroy(&st);
}
static inline DelayedRequest* DelayedRequestStackTrace_create(const JSObject* request) {
  DelayedRequestStackTrace* req = DelayedRequest_create(request, sizeof(DelayedRequestStackTrace), &DelayedRequestStackTrace_handle);
  const JSObject* arguments = JSObject_get_object_field(request, "arguments");
  // Stanza implementatin is single-threaded. Interpret thread_id as coroutine_id?
  req->thread_id = JSObject_get_integer_field(arguments, "threadId", INVALID_THREAD_ID);
  req->start_frame = JSObject_get_integer_field(arguments, "startFrame", 0);
  req->max_frames = JSObject_get_integer_field(arguments, "levels", INT64_MAX);
  return (DelayedRequest*) req;
}
static bool request_stackTrace(const JSObject* request) {
  insert_to_request_queue(DelayedRequestStackTrace_create(request));
  return true;
}

// "ContinueRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "Continue request; value of command field is 'continue'.
//                     The request starts the debuggee to run again.",
//     "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "continue" ]
//       },
//       "arguments": {
//         "$ref": "#/definitions/ContinueArguments"
//       }
//     },
//     "required": [ "command", "arguments"  ]
//   }]
// },
// "ContinueArguments": {
//   "type": "object",
//   "description": "Arguments for 'continue' request.",
//   "properties": {
//     "threadId": {
//       "type": "integer",
//       "description": "Continue execution for the specified thread (if
//                       possible). If the backend cannot continue on a single
//                       thread but will continue on all threads, it should
//                       set the allThreadsContinued attribute in the response
//                       to true."
//     }
//   },
//   "required": [ "threadId" ]
// },
// "ContinueResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'continue' request.",
//     "properties": {
//       "body": {
//         "type": "object",
//         "properties": {
//           "allThreadsContinued": {
//             "type": "boolean",
//             "description": "If true, the continue request has ignored the
//                             specified thread and continued all threads
//                             instead. If this attribute is missing a value
//                             of 'true' is assumed for backward
//                             compatibility."
//           }
//         }
//       }
//     },
//     "required": [ "body" ]
//   }]
// }
typedef DelayedRequest DelayedRequestContinue;
static void DelayedRequestContinue_handle(DelayedRequest* req) {
  // TODO: call LoStanza function here.
  JSBuilder builder;
  JSBuilder_initialize_delayed_response(&builder, req, NULL);
  JSBuilder_object_field_begin(&builder, "body");
  {
    JSBuilder_write_bool_field(&builder, "allThreadsContinued", true);
  }
  JSBuilder_object_field_end(&builder); // body
  JSBuilder_send_and_destroy_response(&builder);
}
static inline DelayedRequest* DelayedRequestContinue_create(const JSObject* request) {
  return DelayedRequest_create(request, sizeof(DelayedRequestContinue), &DelayedRequestContinue_handle);
}
static bool request_continue(const JSObject* request) {
  // TODO: Do we really need this?
  // const JSObject* arguments = JSObject_get_object_field(request, "arguments");
  // const int64_t thread_id = JSObject_get_integer_field(arguments, "threadId", INVALID_THREAD_ID);
  // Remember the thread ID that caused the resume so we can set the
  // "threadCausedFocus" boolean value in the "stopped" events.
  // g_vsc.focus_tid = GetUnsigned(arguments, "threadId", LLDB_INVALID_THREAD_ID);
  insert_to_request_queue(DelayedRequestContinue_create(request));
  return true;
}

// "PauseRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "Pause request; value of command field is 'pause'. The
//     request suspenses the debuggee. The debug adapter first sends the
//     PauseResponse and then a StoppedEvent (event type 'pause') after the
//     thread has been paused successfully.", "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "pause" ]
//       },
//       "arguments": {
//         "$ref": "#/definitions/PauseArguments"
//       }
//     },
//     "required": [ "command", "arguments"  ]
//   }]
// },
// "PauseArguments": {
//   "type": "object",
//   "description": "Arguments for 'pause' request.",
//   "properties": {
//     "threadId": {
//       "type": "integer",
//       "description": "Pause execution for this thread."
//     }
//   },
//   "required": [ "threadId" ]
// },
// "PauseResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'pause' request. This is just an
//     acknowledgement, so no body field is required."
//   }]
// }
typedef DelayedRequest DelayedRequestPause;
static void DelayedRequestPause_handle(DelayedRequest* request) {
  // TODO: call LoStanza function here.
  respond_to_delayed_request(request, NULL);
}
static inline DelayedRequest* DelayedRequestPause_create(const JSObject* request) {
  return DelayedRequest_create(request, sizeof(DelayedRequestPause), &DelayedRequestPause_handle);
}
static bool request_pause(const JSObject* request) {
  // TODO: Do we really need this?
  // const JSObject* arguments = JSObject_get_object_field(request, "arguments");
  // const int64_t thread_id = JSObject_get_integer_field(arguments, "threadId", INVALID_THREAD_ID);
  insert_to_request_queue(DelayedRequestPause_create(request));
  return true;
}

// "DisconnectRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "Disconnect request; value of command field is
//                     'disconnect'.",
//     "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "disconnect" ]
//       },
//       "arguments": {
//         "$ref": "#/definitions/DisconnectArguments"
//       }
//     },
//     "required": [ "command" ]
//   }]
// },
// "DisconnectArguments": {
//   "type": "object",
//   "description": "Arguments for 'disconnect' request.",
//   "properties": {
//     "terminateDebuggee": {
//       "type": "boolean",
//       "description": "Indicates whether the debuggee should be terminated
//                       when the debugger is disconnected. If unspecified,
//                       the debug adapter is free to do whatever it thinks
//                       is best. A client can only rely on this attribute
//                       being properly honored if a debug adapter returns
//                       true for the 'supportTerminateDebuggee' capability."
//     },
//     "restart": {
//       "type": "boolean",
//       "description": "Indicates whether the debuggee should be restart
//                       the process."
//     }
//   }
// },
// "DisconnectResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'disconnect' request. This is just an
//                     acknowledgement, so no body field is required."
//   }]
// }
typedef DelayedRequest DelayedRequestDisconnect;
static void DelayedRequestDisconnect_handle(DelayedRequest* request) {
  // TODO: call LoStanza function here.
  const char* error = NULL;
  JSBuilder builder;
  JSBuilder_initialize_delayed_response(&builder, request, NULL);
  if (error)
    JSBuilder_write_string_field(&builder, "error", error);
  JSBuilder_send_and_destroy_response(&builder);

  if (!error) send_terminated();
}
static inline DelayedRequest* DelayedRequestDisconnect_create(const JSObject* request) {
  return DelayedRequest_create(request, sizeof(DelayedRequestDisconnect), &DelayedRequestDisconnect_handle);
}
static bool request_disconnect(const JSObject* request) {
  // TODO: Do we really need this?
  // const JSObject* arguments = JSObject_get_object_field(request, "arguments");
  // const bool terminate_debugee = JSObject_get_bool_field(arguments, "terminateDebugee", true);
  // const bool restert = JSObject_get_bool_field(arguments, "restart", false);
  insert_to_request_queue(DelayedRequestDisconnect_create(request));
  return true;
}

#define FOR_EACH_REQUEST(def) \
  def(continue)               \
  def(disconnect)             \
  def(initialize)             \
  def(launch)                 \
  def(pause)                  \
  def(setBreakpoints)         \
  def(setExceptionBreakpoints)\
  def(stackTrace)             \
  def(threads)

static const char* const request_names[] = {
  #define DEFINE_REQUEST_NAME(name) #name,
    FOR_EACH_REQUEST(DEFINE_REQUEST_NAME)
  #undef DEFINE_REQUEST_NAME
  NULL
};
// Request handler log the error and returns false for a malformed request.
static bool (*request_handlers[])(const JSObject*) = {
  #define DEFINE_REQUEST_HANDLER(name) request_##name,
    FOR_EACH_REQUEST(DEFINE_REQUEST_HANDLER)
  #undef DEFINE_REQUEST_HANDLER
};
#undef FOR_EACH_REQUEST

static inline bool parse_request(JSValue* value, const char* data, ssize_t length) {
  JSParser parser;
  JSParser_initialize(&parser, data, length);
  if (JSParser_parse_value(&parser, value) && JSParser_skip_spaces(&parser) != parser.limit)
    JSParser_error(&parser, "Extra text after the object end");
  if (parser.error[0]) {
    log_printf("error: failed to parse JSON: %s\n", parser.error);
    return false;
  }
  if (value->kind != JS_OBJECT) {
    log_printf("error: received JSON is not an object\n");
    return false;
  }
  JSObject* object = &value->u.o;
  const char* type = JSObject_get_string_field(object, "type");
  if (!type || strcmp(type, "request")) {
    log_printf("error: received JSON 'type' field is not 'request'\n");
    return false;
  }
  const char* command = JSObject_get_string_field(object, "command");
  if (!command) {
    log_printf("error: 'command' field of 'string' type expected\n");
    return false;
  }
  for (const char* const* p = request_names; *p; p++)
    if (!strcmp(*p, command))                             // request name matches the command?
      return request_handlers[p - request_names](object); // call the request handler, return the result
  log_printf("error: unhandled command '%s'\n");
  return false;
}

static const char* canonicalize_request_name(const char* name) {
  assert(name);
  for (const char* const* p = request_names; *p; p++)
    if (!strcmp(*p, name))
      return *p;
  return NULL;
}

typedef struct {
  int argc;
  char** argv;
} Args;
static inline void next_arg(Args* args) {
  args->argc--;
  args->argv++;
}
static bool is_arg(const char* name, Args* args) {
  const char* arg = args->argv[0];
  if (*arg++ == '-' && *arg++ == '-' && !strcmp(arg, name)) {
    next_arg(args);
    return true;
  }
  return false;
}
static char* get_arg_value(Args* args) {
  if (args->argc) {
    char* value = args->argv[0];
    if (value[0] != '-' && value[1] != '-') {
      next_arg(args);
      return value;
    }
  }
  fprintf(stderr, "Command-line parameter %s must be followed by a value\n", args->argv[0]);
  exit(EXIT_FAILURE);
  return NULL;
}

static inline void initialize_stanza_debugger(int argc, char** argv) {
  // TODO: if necessary, call initialization of stanza debugger here. It runs on communication thread.
}

int main(int argc, char** argv) {
  // Set line buffering mode to stdout and stderr
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  // Allocate and compute absolute path to the adapter
  debug_adapter_path = get_absolute_path(argv[0]);
  Args args = {argc, argv};
  next_arg(&args);// Remove adapter from the args

  const char* log_path = NULL;
  char* port_arg = NULL;

  while (args.argc > 0) {
    if (is_arg("log", &args)) {
      log_path = get_arg_value(&args);
      continue;
    }
    if (is_arg("port", &args)) {
      port_arg = get_arg_value(&args);
      continue;
    }
    break;
  }

#if 0 // Currently unused
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
#endif

  if (log_path) {
    debug_adapter_log = fopen(log_path, "wt");
    if (!debug_adapter_log) {
      fprintf(stderr, "error opening log file \"%s\" (%s)\n", log_path, current_error());
      return EXIT_FAILURE;
    }
    if (setvbuf(debug_adapter_log, NULL, _IOLBF, BUFSIZ)) {
      fprintf(stderr, "error setting line buffering mode for log file (%s)\n", current_error());
      return EXIT_FAILURE;
    }
    if (pthread_mutex_init(&log_lock, NULL)) {
      fprintf(debug_adapter_log, "error: initializing log mutex (%s)\n", current_error());
      return EXIT_FAILURE;
    }
  }

  if (port_arg) {
    char* remainder;
    int port = strtoul(port_arg, &remainder, 0); // Ordinary C notation
    if (*remainder) {
      fprintf(stderr, "'%s' is not a valid port number.\n", port_arg);
      return EXIT_FAILURE;
    }

    printf("Listening on port %i...\n", port);
    SOCKET tmpsock = socket(AF_INET, SOCK_STREAM, 0);
    if (tmpsock < 0) {
      log_printf("error: opening socket (%s)\n", current_error());
      return EXIT_FAILURE;
    }

    {
      struct sockaddr_in serv_addr;
      memset(&serv_addr, 0, sizeof serv_addr);
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      serv_addr.sin_port = htons(port);
      if (bind(tmpsock, (struct sockaddr*)&serv_addr, sizeof serv_addr) < 0) {
        log_printf("error: binding socket (%s)\n", current_error());
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
      log_printf("error: accepting socket (%s)\n", current_error());
      return EXIT_FAILURE;
    }
    debug_adapter_socket = sock;
    debug_adapter_read = &read_from_socket;
    debug_adapter_write = &write_to_socket;
  } else {
    debug_adapter_input_fd = dup(fileno(stdin));
    debug_adapter_output_fd = dup(fileno(stdout));
    debug_adapter_read = &read_from_file;
    debug_adapter_write = &write_to_file;
  }

  if (pthread_mutex_init(&send_lock, NULL)) {
    log_printf("error: initializing send mutex (%s)\n", current_error());
    return EXIT_FAILURE;
  }
  if (pthread_mutex_init(&request_queue_lock, NULL)) {
    log_printf("error: initializing request queue mutex (%s)\n", current_error());
    return EXIT_FAILURE;
  }

  // Initialize debugger with filtered argc, argv
  initialize_stanza_debugger(args.argc, args.argv);

  redirect_output(stdout, STDOUT);
  //redirect_output(stderr, STDERR);

  for (char* data; !sent_terminated_event; free(data)) {
    const ssize_t length = read_packet(&data);
    if (!length) break;

    JSValue received;
    if (!parse_request(&received, data, length)) break;

    // TODO: Handle the received object here
    JSValue_destroy(&received);
  }

  // Terminate debugger

  sleep(1);

  // free_path(debug_adapter_path); // OS will release the process memory
  return EXIT_SUCCESS;
}