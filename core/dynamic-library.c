#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#include <stanza.h>

void* load_library(const stz_byte* name) {
  return (void*)LoadLibrary((LPCSTR)name);
}

stz_int free_library(void* handle) {
  if (!FreeLibrary((HMODULE)handle)) {
    return -1;
  }
  return 0;
}

void* get_proc_address(void* handle, const stz_byte* name) {
  return (void*)GetProcAddress((HMODULE)handle, (LPCSTR)name);
}
#endif
