#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<stdint.h>

#ifdef PLATFORM_WINDOWS
  #include<windows.h>
#else
  #include<sys/mman.h>
#endif

//Declare the external trampoline.
int c_extern_trampoline();

//Declare the settings structucture.
typedef struct {
  uint64_t num_int_args;
  uint64_t num_float_args;
  uint64_t num_mem_args;
  uint64_t extern_index;
  uint64_t entry_point;
  uint64_t index_list;
  uint64_t args_list;
  uint64_t return_type;
} ExternSettings;

extern ExternSettings c_extern_trampoline_params;

//Declare the code template.
extern char c_extern_trampoline_stub_start[];
extern char c_extern_trampoline_stub_end[];

//Fill a hole in the given memory packet.
char* fill_hole (char* mem, char* end, uint64_t replacement){
  while(mem < end){
    uint64_t* hole = (uint64_t*)mem;
    if(*hole == 0xcafebabecafebabe){
      memcpy(hole, &replacement, 8);
      return mem + 8;
    }
    mem++;
  }
  printf("No hole found!\n");
  exit(-1);
}

//Copy over the code template.
char* copy_trampoline_template (char* mem,
                                int num_int_args,
                                int num_float_args,
                                int num_mem_args,
                                int extern_index,
                                int return_type,
                                uint64_t input_indices[]) {
  int num_args = num_int_args + num_float_args + num_mem_args;
  int code_size = c_extern_trampoline_stub_end - c_extern_trampoline_stub_start;
  int template_size = code_size + 8 * num_args;
  memcpy(mem, c_extern_trampoline_stub_start, template_size);
  char* end = mem + template_size;
  uint64_t* indices = (uint64_t*)(mem + code_size);
  mem = fill_hole(mem, end, (uint64_t)&c_extern_trampoline_params);
  mem = fill_hole(mem, end, num_int_args);
  mem = fill_hole(mem, end, num_float_args);
  mem = fill_hole(mem, end, num_mem_args);
  mem = fill_hole(mem, end, extern_index);
  mem = fill_hole(mem, end, return_type);
  mem = fill_hole(mem, end, (uint64_t)&c_extern_trampoline);
  for(int i=0; i<num_int_args + num_float_args + num_mem_args; i++)
    indices[i] = input_indices[i];
  return end;
}

//Argument list.
#define arglist_len 12
uint64_t arglist[arglist_len];

//Fill arglist with zeroes.
void clear_arglist (){
  for(int i=0; i<arglist_len; i++)
    arglist[i] = 0;
}

//Dummy entry point
void print_arglist (int index){
  printf("print_arglist (index = %d)\n", index);
  printf("Contents of arglist:\n");
  for(int i=0; i<arglist_len; i++){
    uint64_t value = arglist[i];
    double d = *(double*)&arglist[i];
    printf("  arglist[%d] = %lld (%g)\n", i, value, d);
  }
  if(index < 5){
    arglist[0] = index * 100 + 1;
  }else{
    double* ret = (double*)&arglist;
    *ret = (double)index * 100 + 0.1;
  }
}

//One int
void func_1i0f (int x){
  //Set the dummy entry point
  uint64_t index_list[] = {0};
  c_extern_trampoline_params.num_int_args = 1;
  c_extern_trampoline_params.num_float_args = 0;
  c_extern_trampoline_params.num_mem_args = 0;
  c_extern_trampoline_params.extern_index = 0;
  c_extern_trampoline_params.return_type = 0;
  c_extern_trampoline_params.index_list = (uint64_t)&index_list;
  c_extern_trampoline_params.args_list = (uint64_t)&arglist;
  c_extern_trampoline_params.entry_point = (uint64_t)&print_arglist;

  //Call the trampoline with (42).
  int (*f1)(int) = c_extern_trampoline;
  int result = f1(x);
  printf("Result = %d\n", result);
}

//One float
void func_0i1f (double x){
  //Set the dummy entry point
  uint64_t index_list[] = {0};
  c_extern_trampoline_params.num_int_args = 0;
  c_extern_trampoline_params.num_float_args = 1;
  c_extern_trampoline_params.num_mem_args = 0;
  c_extern_trampoline_params.extern_index = 1;
  c_extern_trampoline_params.return_type = 0;
  c_extern_trampoline_params.index_list = (uint64_t)&index_list;
  c_extern_trampoline_params.args_list = (uint64_t)&arglist;
  c_extern_trampoline_params.entry_point = (uint64_t)&print_arglist;

  //Call the trampoline with (42).
  int (*f1)(double) = c_extern_trampoline;
  int result = f1(x);
  printf("Result = %d\n", result);
}

//One int, one float.
void func_1i1f (int x, double y){
  //Set the dummy entry point
  uint64_t index_list[] = {0, 1};
  c_extern_trampoline_params.num_int_args = 1;
  c_extern_trampoline_params.num_float_args = 1;
  c_extern_trampoline_params.num_mem_args = 0;
  c_extern_trampoline_params.extern_index = 2;
  c_extern_trampoline_params.return_type = 0;
  c_extern_trampoline_params.index_list = (uint64_t)&index_list;
  c_extern_trampoline_params.args_list = (uint64_t)&arglist;
  c_extern_trampoline_params.entry_point = (uint64_t)&print_arglist;

  //Call the trampoline with (42).
  int (*f1)(int, double) = c_extern_trampoline;
  int result = f1(x, y);
  printf("Result = %d\n", result);
}

void func_1i1f_2 (double x, int y){
  //Set the dummy entry point
  #ifdef PLATFORM_WINDOWS
    uint64_t index_list[] = {1, 0, 0};
    c_extern_trampoline_params.num_int_args = 2;
    c_extern_trampoline_params.num_float_args = 1;
    c_extern_trampoline_params.num_mem_args = 0;
  #else
    uint64_t index_list[] = {1, 0};
    c_extern_trampoline_params.num_int_args = 1;
    c_extern_trampoline_params.num_float_args = 1;
    c_extern_trampoline_params.num_mem_args = 0;
  #endif
  c_extern_trampoline_params.extern_index = 3;
  c_extern_trampoline_params.return_type = 0;
  c_extern_trampoline_params.index_list = (uint64_t)&index_list;
  c_extern_trampoline_params.args_list = (uint64_t)&arglist;
  c_extern_trampoline_params.entry_point = (uint64_t)&print_arglist;

  //Call the trampoline with (42).
  int (*f1)(double, uint64_t) = c_extern_trampoline;
  int result = f1(x, y);
  printf("Result = %d\n", result);
}

//Three ints, three floats
void func_3i3f (int x1, double y1,
                int x2, double y2,
                int x3, double y3){
  //Set the dummy entry point
  #ifdef PLATFORM_WINDOWS
    uint64_t index_list[] = {4, 2, 4, 0, 3, 4, 1, 4, 5, 4};
    c_extern_trampoline_params.num_int_args = 4;
    c_extern_trampoline_params.num_float_args = 4;
    c_extern_trampoline_params.num_mem_args = 2;    
  #else
    uint64_t index_list[] = {4, 2, 0, 5, 3, 1};
    c_extern_trampoline_params.num_int_args = 3;
    c_extern_trampoline_params.num_float_args = 3;
    c_extern_trampoline_params.num_mem_args = 0;
  #endif
  c_extern_trampoline_params.extern_index = 4;
  c_extern_trampoline_params.return_type = 0;
  c_extern_trampoline_params.index_list = (uint64_t)&index_list;
  c_extern_trampoline_params.args_list = (uint64_t)&arglist;
  c_extern_trampoline_params.entry_point = (uint64_t)&print_arglist;

  //Call the trampoline with (42).
  int (*f1)(uint64_t, double, uint64_t, double, uint64_t, double) = c_extern_trampoline;
  int result = f1(x1, y1, x2, y2, x3, y3);
  printf("Result = %d\n", result);
}

//Ints and mems.
void func_6i4m (int x1, int x2, int x3, int x4, int x5, int x6,
                int x7, int x8, int x9, int x10){
  //Set the dummy entry point
  #ifdef PLATFORM_WINDOWS
    uint64_t index_list[] = {3, 2, 1, 0, 9, 8, 7, 6, 5, 4};
    c_extern_trampoline_params.num_int_args = 4;
    c_extern_trampoline_params.num_float_args = 0;
    c_extern_trampoline_params.num_mem_args = 6;
  #else
    uint64_t index_list[] = {5, 4, 3, 2, 1, 0, 9, 8, 7, 6};
    c_extern_trampoline_params.num_int_args = 6;
    c_extern_trampoline_params.num_float_args = 0;
    c_extern_trampoline_params.num_mem_args = 4;
  #endif
  c_extern_trampoline_params.extern_index = 5;
  c_extern_trampoline_params.return_type = 1;
  c_extern_trampoline_params.index_list = (uint64_t)&index_list;
  c_extern_trampoline_params.args_list = (uint64_t)&arglist;
  c_extern_trampoline_params.entry_point = (uint64_t)&print_arglist;

  //Call the trampoline.
  double (*f1)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
               uint64_t, uint64_t, uint64_t, uint64_t) = (void*)c_extern_trampoline;
  double result = f1(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10);
  printf("Result = %g\n", result);
}

//Test calling the set_extern_arguments stub.
void test_set_extern_arguments (){
  printf("Testing set_extern_arguments\n");

  func_1i0f(42);
  func_1i0f(83);
  
  func_0i1f(4.2);
  func_0i1f(-8.3);
  
  func_1i1f(42, -8.3);
  func_1i1f(83, -4.2);
  
  func_1i1f_2(4.2, -83);
  func_1i1f_2(8.3, -42);

  func_3i3f(11, 11.1, 12, 12.2, 13, 13.3);

  clear_arglist();
  func_6i4m(41, 42, 43, 44, 45, 46, 47, 48, 49, 50);
}

char* allocate_exec_memory (int size){
  #ifdef PLATFORM_WINDOWS
    // Reserve the max size with no access
    char* code = VirtualAlloc(NULL, (SIZE_T)size, MEM_RESERVE, PAGE_NOACCESS);

    // Commit the min size with RWX access.
    return VirtualAlloc(code, (SIZE_T)size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  
  #else
    return mmap(0, size,
                PROT_READ|PROT_WRITE|PROT_EXEC,
                MAP_PRIVATE|MAP_ANON, -1, 0);  
  #endif
}

//Test calling the code template.
void test_code_template (){
  printf("Testing code template\n");

  //Configure trampoline.
  c_extern_trampoline_params.args_list = (uint64_t)&arglist;
  c_extern_trampoline_params.entry_point = (uint64_t)&print_arglist;

  //Generate code templates.
  char* code = allocate_exec_memory(1024 * 1024);
  
  int (*f_0i0f)() = (void*)code;
  {
    uint64_t indices[] = {};
    code = copy_trampoline_template(code, 0, 0, 0, 0, 0, indices);
  }

  int (*f_1i0f)(int) = (void*)code;
  {
    uint64_t indices[] = {0};
    code = copy_trampoline_template(code, 1, 0, 0, 1, 0, indices);
  }

  int (*f_1i1f)(int, double) = (void*)code;
  {
    uint64_t indices[] = {0, 1};
    code = copy_trampoline_template(code, 1, 1, 0, 2, 0, indices);
  }

  int (*f_4i2f)(int64_t, int64_t, double, int64_t, int64_t, double) = (void*)code;
  {
    #ifdef PLATFORM_WINDOWS
      uint64_t indices[] = {3, 4, 1, 0, 4, 2, 4, 4, 5, 4};
      code = copy_trampoline_template(code, 4, 4, 2, 3, 0, indices);
    #else
      uint64_t indices[] = {4, 3, 1, 0, 5, 2};
      code = copy_trampoline_template(code, 4, 2, 0, 3, 0, indices);
    #endif
  }

  double (*f_6i4m)(int64_t, int64_t, int64_t, int64_t, int64_t,
                   int64_t, int64_t, int64_t, int64_t, int64_t) = (void*)code;
  {
    #ifdef PLATFORM_WINDOWS
      uint64_t indices[] = {3, 2, 1, 0, 9, 8, 7, 6, 5, 4};
      code = copy_trampoline_template(code, 4, 0, 6, 5, 1, indices);
    #else
      uint64_t indices[] = {5, 4, 3, 2, 1, 0, 9, 8, 7, 6};
      code = copy_trampoline_template(code, 6, 0, 4, 5, 1, indices);
    #endif
  }

  //Call code templates.
  f_0i0f();
  f_1i0f(42);
  f_1i1f(42, 8.3);
  f_4i2f(42, 43, 8.3, -42, -43, -8.3);
  double result = f_6i4m(41, 42, 43, 44, 45, 46, 47, 48, 49, 50);
  printf("f_6i4m result = %g\n", result);
}

int main (int argc, char** argvs){
  clear_arglist();
  test_set_extern_arguments();
  clear_arglist();
  test_code_template();
}
