#ifndef RUNTIME_TYPES_H
#define RUNTIME_TYPES_H
#include <stdint.h>

// These types match the size/signedness of the corresponding Stanza type,
// whereas the size/signedness of the corresponding bare C type (e.g. `char`,
// `int`) may be platform-specific. We use these at the interface between
// Stanza and the runtime to prevent size/signedness mismatch.
typedef int64_t stz_long;
typedef int32_t stz_int;
typedef uint8_t stz_byte;
typedef double  stz_double;
typedef float   stz_float;

// Conversion macros between Stanza-strings (stz_byte*) and C-strings (char*)
// These are necessary because `char` may be signed, but `stz_byte` is unsigned
// It also helps us identify exactly where conversions are occurring
#define C_STR(x)    ((char*)(x))
#define C_CSTR(x)   ((const char*)(x))
#define STZ_STR(x)  ((stz_byte*)(x))
#define STZ_CSTR(x) ((const stz_byte*)(x))

#endif
