#ifndef STANZA_TYPES_H
#define STANZA_TYPES_H
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

// Macro to create an integer literal of size `stz_long`. Using LL is probably
// always OK but using an explicit macro helps keep definition/declarations in
// sync. (compare `stz_long i = 4LL` to `stz_long i = STZ_LONG(4)`)
#ifdef PLATFORM_WINDOWS
#define STZ_LONG(N) ((stz_long)N##LL)
#else
#define STZ_LONG(N) ((stz_long)N##L)
#endif

#endif
