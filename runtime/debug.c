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

static inline void* memclear(void* data, size_t size) {
  return memset(data, 0, size);
}
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
static void log_packet(const char* prefix, const char* data, size_t length) {
  if (debug_adapter_log) {
    pthread_mutex_lock(&log_lock);

    fprintf(debug_adapter_log, "\n%s\nContent-Length: %" PRIu64 "\n\n", prefix, (uint64_t)length);
    fwrite(data, length, 1, debug_adapter_log);
    fprintf(debug_adapter_log, "\n");

    pthread_mutex_unlock(&log_lock);
  }
}

typedef struct {
  size_t length;
  size_t capacity;
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
static bool terminated_event_sent;
static bool stack_trace_available; // Stack trace is not available until VMState initialized
static bool execution_paused;
static pthread_mutex_t send_lock;
static const char* debug_adapter_path;

static inline char* get_absolute_path(const char* s) {
  // Use GetFullPathName on Windows
  return realpath(s, NULL);
}
static inline void free_path(const char* s) {
  free((char*)s);
}

enum {
  NOP = 0x90,
  INT3 = 0xCC
};

typedef const struct {
  uint8_t* const address;
  const uint64_t group;
} SafepointAddress;

typedef const struct {
  const uint64_t length;
  SafepointAddress addresses[];
} AddressList;
static void AddressList_write(AddressList* list, const uint8_t inst) {
  for (SafepointAddress *p = list->addresses, *lim = p + list->length; p < lim; p++)
    *p->address = inst;
}
static inline SafepointAddress* AddressList_find(AddressList* list, const void* pc) {
  for (SafepointAddress *p = list->addresses, *lim = p + list->length; p < lim; p++)
    if (p->address == pc)
      return p;
  return NULL;
}

typedef const struct {
  const uint64_t line;
  AddressList* const address_list;
} SafepointEntry;
static inline void SafepointEntry_write(SafepointEntry* entry, const uint8_t inst) {
  AddressList_write(entry->address_list, inst);
}

typedef const struct {
  const uint64_t num_entries;
  const char* const filename;
  SafepointEntry entries[];
} FileSafepoints;
static void FileSafepoints_write(FileSafepoints* file, const uint8_t inst) {
  for (SafepointEntry *entry = file->entries, *lim = entry + file->num_entries; entry < lim; entry++)
    SafepointEntry_write(entry, inst);
}
static inline SafepointEntry* FileSafepoints_find(FileSafepoints* file, uint64_t line) {
  for (SafepointEntry *entry = file->entries, *lim = entry + file->num_entries; entry < lim; entry++)
    if (entry->line >= line)
      return entry;
  return NULL;
}

typedef const struct {
  const uint64_t num_files;
  FileSafepoints* const files[];
} SafepointTable;
SafepointTable* app_safepoint_table;
static void Safepoints_write(const uint8_t inst) {
  SafepointTable* safepoints = app_safepoint_table;
  if (safepoints)
    for (uint64_t i = 0, num_files = app_safepoint_table->num_files; i < num_files; i++)
      FileSafepoints_write(safepoints->files[i], inst);
}
static FileSafepoints* Safepoints_find_file(const char* file_name) {
  const char* file_path = get_absolute_path(file_name);
  FileSafepoints* result = NULL;
  SafepointTable* safepoints = app_safepoint_table;
  if (safepoints) {
    for (uint64_t i = 0, num_files = app_safepoint_table->num_files; i < num_files && !result; i++) {
      FileSafepoints* p = safepoints->files[i];
      const char* path = get_absolute_path(p->filename);
      if (!strcmp(path, file_path))
        result = p;
      free_path(path);
    }
  }
  free_path(file_path);
  return result;
}

static inline void enable_all_safepoints(void) {
  Safepoints_write(INT3);
}
static inline void disable_all_safepoints(void) {
  Safepoints_write(NOP);
}

typedef struct {
  size_t length;
  size_t capacity;
  SafepointEntry** data;
} SafepointEntryVector;
static inline void SafepointEntryVector_initialize(SafepointEntryVector* v) {
  v->length = 0;
  v->capacity = 16; // Initial vector size
  v->data = malloc(v->capacity * sizeof(v->data[0]));
  //TODO: handle possible OOME
}
static inline void SafepointEntryVector_destroy(SafepointEntryVector* v) {
  free(v->data);
}
static inline SafepointEntry** SafepointEntryVector_allocate(SafepointEntryVector* v) {
  if (v->length == v->capacity) {
    v->capacity <<= 1;
    v->data = realloc(v->data, v->capacity * sizeof(v->data[0]));
    //TODO: handle possible OOM
  }
  return v->data + v->length++;
}
static inline SafepointEntry** SafepointEntryVector_find(const SafepointEntryVector* v, SafepointEntry* entry) {
  for (SafepointEntry **p = v->data, **lim = p + v->length; p < lim; p++)
    if (*p == entry)
      return p;
  return NULL;
}

typedef struct ACTIVE_FILE_SAFEPOINTS {
  struct ACTIVE_FILE_SAFEPOINTS* next;
  FileSafepoints* file;
  SafepointEntry* data[];
} ActiveFileSafepoints;
static ActiveFileSafepoints* active_file_safepoints;
static inline void ActiveFileSafepoints_create(const FileSafepoints* file, const SafepointEntryVector* v) {
  const size_t length = v->length;
  if (length) {
    ActiveFileSafepoints* p = malloc(sizeof(ActiveFileSafepoints) + (length+1)*sizeof(v->data[0]));
    p->next = active_file_safepoints;
    active_file_safepoints = p;
    p->file = file;
    p->data[length] = NULL;
    memcpy(p->data, v->data, length*sizeof(v->data[0]));
  }
}
static inline void ActiveFileSafepoints_destroy(const FileSafepoints* file) {
  for (ActiveFileSafepoints **p = &active_file_safepoints, *q; (q = *p) != NULL; p = &q->next) {
    if (q->file == file) {
      *p = q->next;
      free(q);
      break;
    }
  }
}
static inline void ActiveFileSafepoints_restore(void) {
  for (const ActiveFileSafepoints* p = active_file_safepoints; p; p = p->next)
    for (SafepointEntry* const* entry = p->data; *entry; entry++)
      SafepointEntry_write(*entry, INT3);
}
static inline SafepointEntry* ActiveFileSafepoints_find_entry(const void* pc) {
  for (const ActiveFileSafepoints* p = active_file_safepoints; p; p = p->next)
    for (SafepointEntry* const* entry = p->data; *entry; entry++)
      if (AddressList_find((*entry)->address_list, pc))
        return *entry;
  return NULL;
}

// I/O interface and its implementation over files and sockets.
// Note: I am not sure why is this necessary in LLDB.
static ssize_t (*debug_adapter_read) (char* data, size_t length);
static ssize_t (*debug_adapter_write) (const char* data, size_t length);

static SOCKET debug_adapter_socket;
static ssize_t read_from_socket(char* data, size_t length) {
  const ssize_t bytes_received = recv(debug_adapter_socket, data, length, 0);
#ifdef PLATFORM_WINDOWS
  errno = WSAGetLastError();
#endif
  return bytes_received;
}
static ssize_t write_to_socket(const char* data, size_t length) {
  const ssize_t bytes_sent = send(debug_adapter_socket, data, length, 0);
#ifdef PLATFORM_WINDOWS
  errno = WSAGetLastError();
#endif
  return bytes_sent;
}

static int debug_adapter_input_fd;
static int debug_adapter_output_fd;
static ssize_t read_from_file(char* data, size_t length) {
  return read(debug_adapter_input_fd, data, length);
}
static ssize_t write_to_file(const char* data, size_t length) {
  return write(debug_adapter_output_fd, data, length);
}

static void write_full(const char* data, size_t length) {
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

static bool read_full(char* data, size_t length) {
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
static inline void write_packet(const char* data, const size_t length) {
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
  size_t length = strlen(expected);
  return read_full(buffer, length) && memcmp(buffer, expected, length) == 0;
}
static inline size_t read_unsigned(void) {
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
  size_t length;
  size_t capacity;
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

static inline void JSParser_initialize(JSParser* parser, const char* data, size_t length) {
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
  size_t length = strlen(s) - 1; // First character has already matched
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
  size_t length;
  size_t capacity;
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

static void JSBuilder_ensure_capacity(JSBuilder* builder, size_t size) {
  size_t capacity = builder->capacity;
  const size_t length = builder->length;
  const size_t required_capacity = length + size;
  if (required_capacity <= capacity) return;

  while (capacity < required_capacity)
    capacity <<= 1;
  builder->capacity = capacity;

  char* data = builder->data;
  builder->data = memcpy(malloc(capacity), data, length);
  //TODO: handle possible OOM
  free(data);
}
static char* JSBuilder_allocate(JSBuilder* builder, size_t size) {
  JSBuilder_ensure_capacity(builder, size);
  char* data = builder->data + builder->length;
  builder->length += size;
  return data;
}
static void JSBuilder_append_char(JSBuilder* builder, char c) {
  JSBuilder_allocate(builder, 1)[0] = c;
}
static void JSBuilder_append_text(JSBuilder* builder, const char* data, size_t length) {
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

static void JSBuilder_append_null(JSBuilder* builder) {
  JSBuilder_append_string(builder, "null");
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
static void JSBuilder_write_quoted_text(JSBuilder* builder, const char* data, size_t length) {
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
  if (s)
    JSBuilder_write_quoted_text(builder, s, strlen(s));
  else
    JSBuilder_append_null(builder);
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
static void JSBuilder_write_optional_raw_string_field(JSBuilder* builder, const char* name, const char* value) {
  if (value) JSBuilder_write_raw_string_field(builder, name, value);
}
static void JSBuilder_write_string_field(JSBuilder* builder, const char* name, const char* value) {
  JSBuilder_write_field(builder, name);
  JSBuilder_write_quoted_string(builder, value);
}
static void JSBuilder_write_optional_string_field(JSBuilder* builder, const char* name, const char* value) {
  if (value) JSBuilder_write_string_field(builder, name, value);
}
static void JSBuilder_write_unsigned_field(JSBuilder* builder, const char* name, uint64_t value) {
  JSBuilder_write_field(builder, name);
  JSBuilder_append_unsigned(builder, value);
}
static void JSBuilder_write_optional_unsigned_field(JSBuilder* builder, const char* name, uint64_t value) {
  if (value) JSBuilder_write_unsigned_field(builder, name, value);
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

static void send_thread_stopped(int64_t thread_id, StopReason reason, const char* description, uint64_t breakpoint) {
  execution_paused = true;

  JSBuilder builder;
  JSBuilder_initialize_event(&builder, "stopped");
  JSBuilder_write_raw_string_field(&builder, "reason", stop_reason(reason));
  if (description)
    JSBuilder_write_raw_string_field(&builder, "description", description);
  if (breakpoint) {
    JSBuilder_array_field_begin(&builder, "hitBreakpointIds");
    JSBuilder_next(&builder);
    JSBuilder_append_unsigned(&builder, breakpoint);
    JSBuilder_array_field_end(&builder);
  }
  JSBuilder_write_unsigned_field(&builder, "threadId", thread_id);
  JSBuilder_write_bool_field(&builder, "allThreadsStopped", true);
  //JSBuilder_write_bool_field(&builder, "preserveFocusHint", false);
  //JSBuilder_write_bool_field(&builder, "threadCausedFocus", true);
  JSBuilder_send_and_destroy_event(&builder);
}

int64_t stanza_debugger_current_source_position(const void* p, const char** filename);
//const char* current_file = NULL;
//const int64_t current_line = stanza_debugger_current_source_position(breakpoint_id, &current_file);
void send_thread_stopped_at_breakpoint(const void* pc) {
  char description[64];
  const uint64_t thread_id = 12345678;
  uint64_t breakpoint = (uint64_t) ActiveFileSafepoints_find_entry(pc);
  log_printf("!!! Hit a breakpoint at %p, id %" PRIu64 "\n", pc, breakpoint);
  snprintf(description, sizeof description, "breakpoint %" PRIu64, breakpoint);
  send_thread_stopped(thread_id, STOP_REASON_BREAKPOINT, description, breakpoint);
}

static void send_process_exited(uint64_t exit_code) {
  JSBuilder builder;
  JSBuilder_initialize_event(&builder, "exited");
  JSBuilder_write_unsigned_field(&builder, "exitCode", exit_code);
  JSBuilder_send_and_destroy_event(&builder);
}

static void send_terminated(void) {
  if (!terminated_event_sent) {
    terminated_event_sent = true;
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
static void JSBuilder_write_breakpoint(JSBuilder* builder, uint64_t id, bool verified, const char* path, uint64_t line, uint64_t column) {
  JSBuilder_object_begin(builder);
  JSBuilder_write_unsigned_field(builder, "id", id);
  JSBuilder_write_bool_field(builder, "verified", verified);
  if (path) {
    JSBuilder_write_field(builder, "source");
    JSBuilder_object_begin(builder);
    JSBuilder_write_string_field(builder, "path", path);
    // TODO: In addition to "path" LLDB sets "name" field to basename(path)
    JSBuilder_object_end(builder);
    JSBuilder_write_optional_unsigned_field(builder, "line", line);
    JSBuilder_write_optional_unsigned_field(builder, "column", column);
  }
  JSBuilder_object_end(builder);
}
static void send_breakpoint_changed(uint64_t id, bool verified) {
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

static void send_output(const OutputType out, const char* data, size_t length) {
  assert(length > 0);
  JSBuilder builder;
  JSBuilder_initialize_event(&builder, "output");
  JSBuilder_write_raw_string_field(&builder, "category", output_category(out));
  JSBuilder_write_field(&builder, "output");
  JSBuilder_write_quoted_text(&builder, data, length);
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
  if (terminated_event_sent) return;

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
  JSBuilder_write_optional_string_field(builder, "message", message);
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
    {"supportsValueFormattingOptions", true},
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

int stanza_main(int argc, char** argv);

void next_debug_event(void) {
  do {
    sleep(1);
    handle_pending_debug_requests();
    if (terminated_event_sent) break;
  } while (execution_paused);
}

void stop_at_entry(void) {
  send_thread_stopped(12345678, STOP_REASON_ENTRY, "Stopped at entry", 0);
  next_debug_event();
}

// Skeleton of Stanza debgger. It runs on a separate thread.
// TODO: replace it woth LoStanza function.
static void* handle_launch(void* args) {
  char** argv = (char**) args;
  // TODO: Replace the code below with the debugger run with 'argv'
  log_printf("! Running stanza program %s with arguments:\n", argv[0]);
  for (char** p = argv; *++p != NULL;)
    log_printf("  %s\n", *p);

  int argc = 0;
  for (char** p = argv; *p++; argc++);
  stanza_main(argc, argv);

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
  const char* error = get_string_array(request_arguments, "args", 2, &args);
  if (!error) {
    const char** env = NULL;
    error = get_string_array(request_arguments, "env", 0, &env);
    if (!error) {
      free_path(program_path);
      program_path = get_absolute_path(program);
      args[1] = program_path;
      args[0] = debug_adapter_path;
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

static bool request_setBreakpoints(const JSObject* request) {
  JSBuilder builder;
  JSBuilder_initialize_response(&builder, request, NULL);
  JSBuilder_object_field_begin(&builder, "body");
  {
    JSBuilder_array_field_begin(&builder, "breakpoints");
    {
      const JSObject* arguments = JSObject_get_object_field(request, "arguments");
      const JSObject* source = JSObject_get_object_field(arguments, "source");
      const char* path = JSObject_get_string_field(source, "path");
      FileSafepoints* safepoints = Safepoints_find_file(path);
      if (safepoints) {
        ActiveFileSafepoints_destroy(safepoints);
        FileSafepoints_write(safepoints, NOP);  // Clear all safeponts in the file
        const JSArray* breakpoints = JSObject_get_array_field(arguments, "breakpoints");
        if (breakpoints) {
          SafepointEntryVector v;
          SafepointEntryVector_initialize(&v);
          for (const JSValue *p = breakpoints->data, *const limit = p + breakpoints->length; p < limit; p++) {
            if (p->kind == JS_OBJECT) {
              const JSObject* o = &p->u.o;
              uint64_t line = JSObject_get_integer_field(o, "line", 0);
              // const int64_t column = JSObject_get_integer_field(o, "column", 0);
              // TODO: get optional condition, hitCondition and logMessage fields.
              SafepointEntry* entry = FileSafepoints_find(safepoints, line);
              if (entry) {
                line = entry->line;
                if (!SafepointEntryVector_find(&v, entry)) {
                  *SafepointEntryVector_allocate(&v) = entry;
                  SafepointEntry_write(entry, INT3);
                }
              }
              JSBuilder_next(&builder);
              JSBuilder_write_breakpoint(&builder, (uint64_t)entry, entry != NULL, path, line, 0);
            }
          }
          ActiveFileSafepoints_create(safepoints, &v);
          SafepointEntryVector_destroy(&v);
        }
      }
    }
    JSBuilder_array_field_end(&builder); // breakpoints
  }
  JSBuilder_object_field_end(&builder); // body
  JSBuilder_send_and_destroy_response(&builder);
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

// "SourceRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "Source request; value of command field is 'source'. The
//     request retrieves the source code for a given source reference.",
//     "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "source" ]
//       },
//       "arguments": {
//         "$ref": "#/definitions/SourceArguments"
//       }
//     },
//     "required": [ "command", "arguments"  ]
//   }]
// },
// "SourceArguments": {
//   "type": "object",
//   "description": "Arguments for 'source' request.",
//   "properties": {
//     "source": {
//       "$ref": "#/definitions/Source",
//       "description": "Specifies the source content to load. Either
//       source.path or source.sourceReference must be specified."
//     },
//     "sourceReference": {
//       "type": "integer",
//       "description": "The reference to the source. This is the same as
//       source.sourceReference. This is provided for backward compatibility
//       since old backends do not understand the 'source' attribute."
//     }
//   },
//   "required": [ "sourceReference" ]
// },
// "SourceResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'source' request.",
//     "properties": {
//       "body": {
//         "type": "object",
//         "properties": {
//           "content": {
//             "type": "string",
//             "description": "Content of the source reference."
//           },
//           "mimeType": {
//             "type": "string",
//             "description": "Optional content type (mime type) of the source."
//           }
//         },
//         "required": [ "content" ]
//       }
//     },
//     "required": [ "body" ]
//   }]
// }
static bool request_source(const JSObject* request) {
  respond_to_request(request, "Source file not found");
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
enum {
  INVALID_THREAD_ID = -1,
  INVALID_FRAME_ID = -1
};

#define DEFINE_STACK_FRAME_FORMAT(def)\
  def(0, hex)            \
  def(1, parameters)     \
  def(2, parameterTypes) \
  def(3, parameterNames) \
  def(4, parameterValues)\
  def(5, line)           \
  def(6, module)         \
  def(7, includeAll)
enum {
  #define DEFINE_STACK_FRAME_FORMAT_MASK(number, name) STACK_FRAME_FORMAT_##name = 1 << number,
  DEFINE_STACK_FRAME_FORMAT(DEFINE_STACK_FRAME_FORMAT_MASK)
  #undef DEFINE_STACK_FRAME_FORMAT_MASK
};
static inline uint64_t parse_stack_frame_format(const JSObject* format) {
  static const char* const names[] = {
    #define DEFINE_STACK_FRAME_FORMAT_NAME(number, name) #name,
    DEFINE_STACK_FRAME_FORMAT(DEFINE_STACK_FRAME_FORMAT_NAME)
    #undef DEFINE_STACK_FRAME_FORMAT_NAME
    NULL
  };

  uint64_t x = 0;
  uint64_t mask = 1;
  for (const char* const* name = names; *name; name++, mask <<= 1)
    if (JSObject_get_bool_field(format, *name, false))
      x |= mask;
  return x;
}
typedef struct {
  char* function_name;  // Owned by StackTraceFrame
  char* source_path;    // Owned by StackTraceFrame
  int64_t line;         // 1-based
  int64_t column;       // 1-based, 0 denotes unknown
  uint64_t pc;
  uint64_t sp;
} StackTraceFrame;
typedef struct {
  size_t length;
  size_t capacity;
  StackTraceFrame* data;
} StackTrace;
static inline void StackTrace_initialize(StackTrace* st) {
  st->length = 0;
  st->capacity = 16; // Initial stack trace size
  st->data = malloc(st->capacity * sizeof(StackTraceFrame));
  //TODO: handle possible OOME
}
static inline void StackTrace_destroy(StackTrace* st) {
  for (const StackTraceFrame *p = st->data, *const limit = p + st->length; p < limit; p++) {
    free(p->function_name);
    free(p->source_path);
  }
  free(st->data);
}
static inline void StackTrace_reset(StackTrace* st) {
  StackTrace_destroy(st);
  StackTrace_initialize(st);
}
static inline StackTraceFrame* StackTrace_allocate(StackTrace* st) {
  if (st->length == st->capacity) {
    st->capacity <<= 1;
    st->data = realloc(st->data, st->capacity * sizeof(StackTraceFrame));
    //TODO: handle possible OOM
  }
  return st->data + st->length++;
}
static StackTrace current_stack_trace;

void append_stack_frame(uint64_t pc, uint64_t sp, const char* function_name,
                        const char* source_path, int64_t line, int64_t column) {
  StackTraceFrame* frame = StackTrace_allocate(&current_stack_trace);
  frame->function_name = strdup(function_name);
  frame->source_path = strdup(source_path);
  frame->line = line;
  frame->column = column;
  frame->pc = pc;
  frame->sp = sp;
}

// Create stack trace for given thread_id starting at start level with no more than max_frames frames.
// Frames not visible to the debugger can be skipped.
// Returns total number of frames in thread_id stack or -1 when no thread with given thread_id thread_is found.
int32_t create_stack_trace(const uint64_t format) ;

typedef struct {
  DelayedRequest parent;
  int64_t thread_id;
  int64_t start_frame;
  int64_t max_frames;
  uint64_t format;
} DelayedRequestStackTrace;
static void DelayedRequestStackTrace_handle(DelayedRequest* request) {
  const DelayedRequestStackTrace* req = (const DelayedRequestStackTrace*)request;
  StackTrace_reset(&current_stack_trace);
  if (!terminated_event_sent && stack_trace_available) create_stack_trace(req->format);
  const int64_t total_frames = current_stack_trace.length;

  JSBuilder builder;
  JSBuilder_initialize_delayed_response(&builder, request, NULL);
  JSBuilder_object_field_begin(&builder, "body");
  if (total_frames >= 0) {
    JSBuilder_write_unsigned_field(&builder, "totalFrames", total_frames);
    JSBuilder_array_field_begin(&builder, "stackFrames");
    for (const StackTraceFrame *p = current_stack_trace.data, *const limit = p + total_frames; p < limit; p++) {
      JSBuilder_next(&builder);
      JSBuilder_object_begin(&builder);
      {
        JSBuilder_write_unsigned_field(&builder, "id", (uint64_t)p);
        JSBuilder_write_string_field(&builder, "name", p->function_name);
        if (p->source_path) {
          JSBuilder_object_field_begin(&builder, "source");
          {
            JSBuilder_write_string_field(&builder, "path", p->source_path);
          }
          JSBuilder_object_field_end(&builder);
        }
        JSBuilder_write_unsigned_field(&builder, "line", p->line);
        JSBuilder_write_unsigned_field(&builder, "column", p->column);  //Mandatory here
      }
      JSBuilder_object_end(&builder);
    }
    JSBuilder_array_field_end(&builder); // stackFrames
  }
  JSBuilder_object_field_end(&builder); // body
  JSBuilder_send_and_destroy_response(&builder);
}
static inline DelayedRequest* DelayedRequestStackTrace_create(const JSObject* request) {
  DelayedRequestStackTrace* req = DelayedRequest_create(request, sizeof(DelayedRequestStackTrace), &DelayedRequestStackTrace_handle);
  const JSObject* arguments = JSObject_get_object_field(request, "arguments");
  // Stanza implementatin is single-threaded. Interpret thread_id as coroutine_id?
  req->thread_id = JSObject_get_integer_field(arguments, "threadId", INVALID_THREAD_ID);
  req->start_frame = JSObject_get_integer_field(arguments, "startFrame", 0);
  req->max_frames = JSObject_get_integer_field(arguments, "levels", INT64_MAX);
  req->format = parse_stack_frame_format(JSObject_get_object_field(arguments, "format"));
  return (DelayedRequest*) req;
}
static bool request_stackTrace(const JSObject* request) {
  insert_to_request_queue(DelayedRequestStackTrace_create(request));
  return true;
}

// "ScopesRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "Scopes request; value of command field is 'scopes'. The
//     request returns the variable scopes for a given stackframe ID.",
//     "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "scopes" ]
//       },
//       "arguments": {
//         "$ref": "#/definitions/ScopesArguments"
//       }
//     },
//     "required": [ "command", "arguments"  ]
//   }]
// },
// "ScopesArguments": {
//   "type": "object",
//   "description": "Arguments for 'scopes' request.",
//   "properties": {
//     "frameId": {
//       "type": "integer",
//       "description": "Retrieve the scopes for this stackframe."
//     }
//   },
//   "required": [ "frameId" ]
// },
// "ScopesResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'scopes' request.",
//     "properties": {
//       "body": {
//         "type": "object",
//         "properties": {
//           "scopes": {
//             "type": "array",
//             "items": {
//               "$ref": "#/definitions/Scope"
//             },
//             "description": "The scopes of the stackframe. If the array has
//             length zero, there are no scopes available."
//           }
//         },
//         "required": [ "scopes" ]
//       }
//     },
//     "required": [ "body" ]
//   }]
// }

// The debugger must be aware of the scope ids
#define VARIABLE_SCOPES(def)\
  def(LOCALS,     "Locals",     "locals"    )\
  def(GLOBALS,    "Globals",    "globals"   )
enum {
  INITIAL_SCOPE,  // O.P: It looks like VSCode requires strictly positive scope id
  #define DEF_SCOPE_ID(id, name, hint) SCOPE_ID_##id,
    VARIABLE_SCOPES(DEF_SCOPE_ID)
  #undef DEF_SCOPE_ID
  NUMBER_OF_SCOPES
};
uint32_t number_of_variables_in_scope(const uint32_t scope_id, const uint64_t pc);

// Varaiable attributes (must be knwn to the debugger)
enum {
  VARIABLE_PUBLIC = 1,
  VARIABLE_PRIVATE = 2,
  VARIABLE_PROTECTED = 3,
  VARIABLE_VISIBILITY_MASK = 3,
  VARIABLE_CONSTANT = 1 << 2
};
static inline const char* variable_visibility(const unsigned variable_attributes) {
  static const char* const names[] = {
    NULL,
    "public",
    "private",
    "protected"
  };
  return names[variable_attributes & VARIABLE_VISIBILITY_MASK];
}

typedef struct {
  char* name;           // Owned by Variable
  char* type;           // Owned by Variable
  char* value;          // Owned by Variable
  const void* ref;
  size_t fields_count;
  size_t fields_base;
  uint8_t attributes;
} Variable;
static inline Variable* Variable_initialize(Variable* var, const void* ref, const char* name, size_t fields_count) {
  var->name = strdup(name);
  var->type = NULL;
  var->value = NULL;
  var->ref = ref;
  var->fields_count = fields_count;
  var->fields_base = 0;
  var->attributes = 0;
  return var;
}
static inline void Variable_destroy(const Variable* var) {
  free(var->name);
  free(var->type);
  free(var->value);
}
static void JSBuilder_write_variable_fields_count(JSBuilder* builder, const Variable* var) {
  JSBuilder_write_optional_unsigned_field(builder, "namedVariables", var->fields_count);
}
static inline void JSBuilder_write_variable(JSBuilder* builder, const Variable* var, const uint64_t var_id) {
  JSBuilder_object_begin(builder);
  {
    // Variable with zero fields must have zero id.
    JSBuilder_write_unsigned_field(builder, "variablesReference", var->fields_count ? var_id : 0);
    JSBuilder_write_raw_string_field(builder, "name", var->name);
    JSBuilder_write_optional_string_field(builder, "type", var->type);
    JSBuilder_write_string_field(builder, "value", var->value);
    JSBuilder_write_variable_fields_count(builder, var);

    const unsigned attributes = var->attributes;
    if (attributes) {
      JSBuilder_object_field_begin(builder, "presentationHint");
      {
        if (attributes & VARIABLE_CONSTANT)
          JSBuilder_write_raw_string_field(builder, "attributes", "constant");
        JSBuilder_write_optional_raw_string_field(builder, "visibility", variable_visibility(attributes));
      }
      JSBuilder_object_field_end(builder);
    }
  }
  JSBuilder_object_end(builder);
}

typedef struct {
  size_t length;
  size_t capacity;
  Variable* data;
} Environment;
static inline void Environment_initialize(Environment* env) {
  env->length = 0;
  env->capacity = 64;
  env->data = malloc(env->capacity * sizeof(Variable));
}
static void Environment_destroy(Environment* env) {
  // It is safe to destroy uninitialized Environment:
  // by default all fields are zeroed
  for (Variable *p = env->data, *limit = p + env->length; p < limit; p++)
    Variable_destroy(p);
  free(env->data);
}
static inline void Environment_reset(Environment* env) {
  Environment_destroy(env);
  Environment_initialize(env);
}
static Variable* Environment_allocate(Environment* env, const void* ref, const char* name, size_t fields_count) {
  if (env->length == env->capacity) {
    env->capacity <<= 1;
    env->data = realloc(env->data, env->capacity * sizeof(Variable));
    //TODO: handle possible OOM
  }
  return Variable_initialize(env->data + env->length++, ref, name, fields_count);
}

static Environment current_frame_env; // Belongs to the app thread
static int64_t current_frame_id = INVALID_FRAME_ID;
static inline uint64_t current_pc(void) {
  return current_frame_id != INVALID_FRAME_ID ? ((const StackTraceFrame*)current_frame_id)->pc : 0;
}
static inline uint64_t current_sp(void) {
  return current_frame_id != INVALID_FRAME_ID ? ((const StackTraceFrame*)current_frame_id)->sp : 0;
}

typedef struct {
  DelayedRequest parent;
  int64_t frame_id;
} DelayedRequestScopes;
static void DelayedRequestScopes_handle(DelayedRequest* request) {
  const DelayedRequestScopes* req = (const DelayedRequestScopes*)request;
  current_frame_id = req->frame_id;
  Environment_reset(&current_frame_env);
  Environment_allocate(&current_frame_env, NULL, "Root scope", NUMBER_OF_SCOPES);

  JSBuilder builder;
  JSBuilder_initialize_delayed_response(&builder, request, NULL);
  JSBuilder_object_field_begin(&builder, "body");
  {
    JSBuilder_array_field_begin(&builder, "scopes");
    while (current_frame_env.length < NUMBER_OF_SCOPES) {
      JSBuilder_next(&builder);
      JSBuilder_object_begin(&builder);
      {
        static const struct {const char* name; const char* hint; } scope_attributes[] = {
          #define DEF_SCOPE_ATTRIBUTES(id, name, hint) {name, hint},
            VARIABLE_SCOPES(DEF_SCOPE_ATTRIBUTES)
          #undef DEF_SCOPE_ATTRIBUTES
        };
        const size_t scope_id = current_frame_env.length;
        const char* scope_name = scope_attributes[scope_id - 1].name;
        const char* scope_hint = scope_attributes[scope_id - 1].hint;
        Variable* var = Environment_allocate(&current_frame_env, (const void*)scope_id, scope_name,
          number_of_variables_in_scope(scope_id, current_pc()));
        JSBuilder_write_raw_string_field(&builder, "name", scope_name);
        JSBuilder_write_raw_string_field(&builder, "presentationHint", scope_hint);
        JSBuilder_write_unsigned_field(&builder, "variablesReference", scope_id);
        JSBuilder_write_variable_fields_count(&builder, var);
        JSBuilder_write_bool_field(&builder, "expensive", false);
      }
      JSBuilder_object_end(&builder);
    }
    JSBuilder_array_field_end(&builder);
  }
  JSBuilder_object_field_end(&builder); // body
  JSBuilder_send_and_destroy_response(&builder);
}
#undef VARIABLE_SCOPES

static inline bool is_valid_stack_trace_frame_id(const uint64_t frame_id) {
  const uint64_t offset = frame_id - (uint64_t)current_stack_trace.data;
  if (offset % sizeof(StackTraceFrame) == 0) {
    const uint64_t index = offset / sizeof(StackTraceFrame);
    return index < (uint64_t)current_stack_trace.length;
  }
  return false;
}
static inline DelayedRequest* DelayedRequestScopes_create(const JSObject* request) {
  DelayedRequestScopes* req = DelayedRequest_create(request, sizeof(DelayedRequestScopes), &DelayedRequestScopes_handle);
  const JSObject* arguments = JSObject_get_object_field(request, "arguments");
  int64_t frame_id = JSObject_get_integer_field(arguments, "frameId", INVALID_FRAME_ID);
  if (frame_id != INVALID_FRAME_ID && !is_valid_stack_trace_frame_id(frame_id))
    frame_id = INVALID_FRAME_ID;
  req->frame_id = frame_id;
  return (DelayedRequest*) req;
}
static bool request_scopes(const JSObject* request) {
  // O.P.: Following comment is shamelessly borrowed from LLDB:
  // As the user selects different stack frames in the GUI, a "scopes" request
  // will be sent to the DAP. This is the only way we know that the user has
  // selected a frame in a thread. There are no other notifications that are
  // sent and VS code doesn't allow multiple frames to show variables
  // concurrently. If we select the thread and frame as the "scopes" requests
  // are sent, this allows users to type commands in the debugger console
  // with a backtick character to run lldb commands and these lldb commands
  // will now have the right context selected as they are run. If the user
  // types "`bt" into the debugger console and we had another thread selected
  // in the LLDB library, we would show the wrong thing to the user. If the
  // users switches threads with a lldb command like "`thread select 14", the
  // GUI will not update as there are no "event" notification packets that
  // allow us to change the currently selected thread or frame in the GUI that
  // I am aware of.

  insert_to_request_queue(DelayedRequestScopes_create(request));
  return true;
}

// "VariablesRequest": {
//   "allOf": [ { "$ref": "#/definitions/Request" }, {
//     "type": "object",
//     "description": "Variables request; value of command field is 'variables'.
//     Retrieves all child variables for the given variable reference. An
//     optional filter can be used to limit the fetched children to either named
//     or indexed children.", "properties": {
//       "command": {
//         "type": "string",
//         "enum": [ "variables" ]
//       },
//       "arguments": {
//         "$ref": "#/definitions/VariablesArguments"
//       }
//     },
//     "required": [ "command", "arguments"  ]
//   }]
// },
// "VariablesArguments": {
//   "type": "object",
//   "description": "Arguments for 'variables' request.",
//   "properties": {
//     "variablesReference": {
//       "type": "integer",
//       "description": "The Variable reference."
//     },
//     "filter": {
//       "type": "string",
//       "enum": [ "indexed", "named" ],
//       "description": "Optional filter to limit the child variables to either
//       named or indexed. If ommited, both types are fetched."
//     },
//     "start": {
//       "type": "integer",
//       "description": "The index of the first variable to return; if omitted
//       children start at 0."
//     },
//     "count": {
//       "type": "integer",
//       "description": "The number of variables to return. If count is missing
//       or 0, all variables are returned."
//     },
//     "format": {
//       "$ref": "#/definitions/ValueFormat",
//       "description": "Specifies details on how to format the Variable
//       values."
//     }
//   },
//   "required": [ "variablesReference" ]
// },
// "VariablesResponse": {
//   "allOf": [ { "$ref": "#/definitions/Response" }, {
//     "type": "object",
//     "description": "Response to 'variables' request.",
//     "properties": {
//       "body": {
//         "type": "object",
//         "properties": {
//           "variables": {
//             "type": "array",
//             "items": {
//               "$ref": "#/definitions/Variable"
//             },
//             "description": "All (or a range) of variables for the given
//             variable reference."
//           }
//         },
//         "required": [ "variables" ]
//       }
//     },
//     "required": [ "body" ]
//   }]
// }
void append_variable(const void* ref, const char* name, const char* type, const char* value,
                     uint64_t fields_count, uint8_t attributes) {
  Variable* v = Environment_allocate(&current_frame_env, ref, name, fields_count);
  v->type = strdup(type);
  v->value = strdup(value);
  v->attributes = attributes;
  v->ref = ref;
}
void create_fields(const void* ref, const char* name, uint64_t fields_count, uint64_t hex,
                   uint64_t pc, uint64_t sp);

typedef struct {
  DelayedRequest parent;
  uint64_t variable_id;
  bool hex;
} DelayedRequestVariables;
static void DelayedRequestVariables_handle(DelayedRequest* request) {
  const DelayedRequestVariables* req = (const DelayedRequestVariables*)request;

  const uint64_t variable_id = req->variable_id;
  if (variable_id >= (uint64_t)current_frame_env.length) {  // Sanity check for variable id
    respond_to_delayed_request(request, "Invalid variable reference");
    return;
  }

  Variable* var = current_frame_env.data + variable_id;
  size_t fields_base = var->fields_base;
  const uint64_t fields_count = var->fields_count;
  if (fields_count && !fields_base) {
    fields_base = current_frame_env.length;
    var->fields_base = fields_base;
    create_fields(var->ref, var->name, fields_count, req->hex, current_pc(), current_sp());
    var = NULL; // Nuke 'var' as it could become invalid when 'env.data' expands.
  }

  JSBuilder builder;
  JSBuilder_initialize_delayed_response(&builder, request, NULL);
  JSBuilder_object_field_begin(&builder, "body");
  {
    JSBuilder_array_field_begin(&builder, "variables");
    const Variable* data = current_frame_env.data;
    for (uint64_t i = fields_base, limit = i + fields_count; i < limit; i++) {
      JSBuilder_next(&builder);
      JSBuilder_write_variable(&builder, data + i, i);
    }
    JSBuilder_array_field_end(&builder);
  }
  JSBuilder_object_field_end(&builder); // body
  JSBuilder_send_and_destroy_response(&builder);
}
static inline DelayedRequest* DelayedRequestVariables_create(const JSObject* request) {
  DelayedRequestVariables* req = DelayedRequest_create(request, sizeof(DelayedRequestVariables), &DelayedRequestVariables_handle);
  const JSObject* arguments = JSObject_get_object_field(request, "arguments");
  req->variable_id = JSObject_get_integer_field(arguments, "variablesReference", -1);
  const JSObject* format = JSObject_get_object_field(arguments, "format");
  req->hex = JSObject_get_bool_field(format, "hex", false);
  return (DelayedRequest*) req;
}
static bool request_variables(const JSObject* request) {
  insert_to_request_queue(DelayedRequestVariables_create(request));
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
int stanza_debugger_continue (void);
static void DelayedRequestContinue_handle(DelayedRequest* req) {
  execution_paused = false;
  stack_trace_available = true;
  stanza_debugger_continue();

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
// }s
typedef DelayedRequest DelayedRequestPause;
int stanza_debugger_pause(void);
static void DelayedRequestPause_handle(DelayedRequest* request) {
  // TODO: call LoStanza function here.
  // Just to simuate the effect:
  stanza_debugger_pause();
  respond_to_delayed_request(request, NULL);
  // TODO: must be sent from the actually paused stanza program.
  send_thread_stopped(12345678, STOP_REASON_PAUSE, "Paused", 0);
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
  JSBuilder_write_optional_string_field(&builder, "error", error);
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
  def(scopes)                 \
  def(setBreakpoints)         \
  def(setExceptionBreakpoints)\
  def(source)                 \
  def(stackTrace)             \
  def(threads)                \
  def(variables)

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

static inline bool parse_request(JSValue* value, const char* data, size_t length) {
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
      memclear(&serv_addr, sizeof serv_addr);
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

  for (char* data; !terminated_event_sent; free(data)) {
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