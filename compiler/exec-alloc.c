#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

#ifdef PLATFORM_WINDOWS
  #include<windows.h>
#else
  #include<sys/mman.h>
#endif

//Print a message and halt immediately.
void allocate_exec_error () {
  printf("Could not allocate executable memory.\n");
  exit(-1);  
}

//Return some executable memory of the given size.
char* allocate_exec_memory (uint64_t size){
  #ifdef PLATFORM_WINDOWS
    // Reserve the max size with no access
    char* code = VirtualAlloc(NULL, (SIZE_T)size, MEM_RESERVE, PAGE_NOACCESS);
    if(!code) allocate_exec_error();

    // Commit the min size with RWX access.
    code = VirtualAlloc(code, (SIZE_T)size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if(!code) allocate_exec_error();

    return code;
  
  #else
    char* code = mmap(0, size,
                      PROT_READ|PROT_WRITE|PROT_EXEC,
                      MAP_PRIVATE|MAP_ANON, -1, 0);  
    if(!code) allocate_exec_error();
    
    return code;
    
  #endif
}
