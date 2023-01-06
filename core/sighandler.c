//Platform-specific defines.
#if defined(PLATFORM_LINUX)
  //Needs to be defined to access REG_RIP.
  #define _GNU_SOURCE
#endif

#include<ucontext.h>
#include<signal.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>

//============================================================
//==================== Machine Types =========================
//============================================================

typedef struct{
  uint64_t current_stack;
  uint64_t system_stack;
  char* top;
  char* limit;
  char* start;
  uint64_t* collection_start;
  uint64_t* bitset;
  uint64_t* bitset_base;
  uint64_t size;
  uint64_t size_limit;
  uint64_t max_size;
  uint64_t* marking_stack_start;
  uint64_t* marking_stack_bottom;
  uint64_t* marking_stack_top;
  char* compaction_start;
  char* min_incomplete;
  char* max_incomplete;
  struct Stack* stacks;
  uint64_t* free_stacks;
  void* liveness_trackers;
  void* iterate_roots;
  void* iterate_references_in_stack_frames;
} Heap;

//The first fields in VMState are used by the core library
//in both compiled and interpreted mode. The last fields
//are used only in interpreted mode.
//Permanent state changes in-between each code load.
//Variable state changes in-between each boundary change.
typedef struct{
  uint64_t* global_offsets;    //(Permanent State)
  char* global_mem;            //(Permanent State)
  uint64_t signal_handler;     //(Variable State)
  uint64_t* const_table;       //(Permanent State)
  char* const_mem;             //(Permanent State)
  uint32_t* data_offsets;      //(Permanent State)
  char* data_mem;              //(Permanent State)
  uint64_t* code_offsets;      //(Permanent State)
  uint64_t* registers;         //(Permanent State)
  uint64_t* system_registers;  //(Permanent State)
  Heap heap;
  uint64_t* class_table;       //(Permanent State)
  //Interpreted Mode Tables
  char* instructions;          //(Permanent State)
  void** trie_table;           //(Permanent State)
} VMState;

typedef struct{
  uint64_t returnpc;
  uint64_t liveness_map;
  uint64_t slots[];
} StackFrame;

typedef struct{
  uint64_t num_slots;
  uint64_t code;
  uint64_t slots[];
} Function;

typedef struct{
  uint64_t size;
  StackFrame* frames;
  StackFrame* stack_pointer;
  uint64_t pc;
  struct Stack* tail;
} Stack;

//============================================================
//============= Saved Signal Handler Context =================
//============================================================

//Saved context for the sigaction_handler.
struct sigaction_context{
  uint64_t rip;
  uint64_t rsp;
  uint64_t crsp;
  uint64_t regs[15];
  uint64_t fregs[16];
};

//------------------------------------------------------------
//----------------- OS-X Implementation ----------------------
//------------------------------------------------------------

#if defined(PLATFORM_OS_X)

//Helper: Retrieve the low 64 bits of the given xmm register.
uint64_t xmm_low_bits (struct __darwin_xmm_reg* xmm){
  uint64_t* p = (uint64_t*)xmm;
  return p[1];
}

//Save the contents of 'context' to the location 'save_context'.
void save_sigaction_context (struct sigaction_context* save_context,
                             void* input_context,
                             uint64_t stanza_crsp){
  ucontext_t* context = (ucontext_t*)input_context;

  save_context->rip = context->uc_mcontext->__ss.__rip;
  save_context->rsp = context->uc_mcontext->__ss.__rsp;
  save_context->crsp = stanza_crsp;
  save_context->regs[0] = context->uc_mcontext->__ss.__rax;
  save_context->regs[1] = context->uc_mcontext->__ss.__rbx;
  save_context->regs[2] = context->uc_mcontext->__ss.__rcx;
  save_context->regs[3] = context->uc_mcontext->__ss.__rdx;
  save_context->regs[4] = context->uc_mcontext->__ss.__rsi;
  save_context->regs[5] = context->uc_mcontext->__ss.__rdi;
  save_context->regs[6] = context->uc_mcontext->__ss.__rbp;
  save_context->regs[7] = context->uc_mcontext->__ss.__r8;
  save_context->regs[8] = context->uc_mcontext->__ss.__r9;
  save_context->regs[9] = context->uc_mcontext->__ss.__r10;
  save_context->regs[10] = context->uc_mcontext->__ss.__r11;
  save_context->regs[11] = context->uc_mcontext->__ss.__r12;
  save_context->regs[12] = context->uc_mcontext->__ss.__r13;
  save_context->regs[13] = context->uc_mcontext->__ss.__r14;
  save_context->regs[14] = context->uc_mcontext->__ss.__r15;
  save_context->fregs[0] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm0);
  save_context->fregs[1] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm1);
  save_context->fregs[2] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm2);
  save_context->fregs[3] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm3);
  save_context->fregs[4] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm4);
  save_context->fregs[5] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm5);
  save_context->fregs[6] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm6);
  save_context->fregs[7] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm7);
  save_context->fregs[8] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm8);
  save_context->fregs[9] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm9);
  save_context->fregs[10] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm10);
  save_context->fregs[11] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm11);
  save_context->fregs[12] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm12);
  save_context->fregs[13] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm13);
  save_context->fregs[14] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm14);
  save_context->fregs[15] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm15);
}

//Write the given attributes to the processor context.
void load_sigaction_context (void* input_context,
                             uint64_t stack_pointer,
                             uint64_t arg0,
                             uint64_t arg1,
                             uint64_t instruction_pointer){
  ucontext_t* context = (ucontext_t*)input_context;
  context->uc_mcontext->__ss.__rsp = stack_pointer;
  context->uc_mcontext->__ss.__rcx = arg0;
  context->uc_mcontext->__ss.__rdx = arg1;
  context->uc_mcontext->__ss.__rip = instruction_pointer;
}

//------------------------------------------------------------
//----------------- Linux Implementation ---------------------
//------------------------------------------------------------

#elif defined(PLATFORM_LINUX)

//Helper: Retrieve the low 64 bits of the given xmm register.
uint64_t xmm_low_bits (struct _libc_xmmreg* xmm){
  uint64_t* p = (uint64_t*)xmm;
  return p[1];
}

//Save the contents of 'context' to the location 'save_context'.
void save_sigaction_context (struct sigaction_context* save_context,
                             void* input_context,
                             uint64_t stanza_crsp){
  ucontext_t* context = (ucontext_t*)input_context;

  save_context->rip = context->uc_mcontext.gregs[REG_RIP];
  save_context->rsp = context->uc_mcontext.gregs[REG_RSP];
  save_context->crsp = stanza_crsp;
  save_context->regs[0] = context->uc_mcontext.gregs[REG_RAX];
  save_context->regs[1] = context->uc_mcontext.gregs[REG_RBX];
  save_context->regs[2] = context->uc_mcontext.gregs[REG_RCX];
  save_context->regs[3] = context->uc_mcontext.gregs[REG_RDX];
  save_context->regs[4] = context->uc_mcontext.gregs[REG_RSI];
  save_context->regs[5] = context->uc_mcontext.gregs[REG_RDI];
  save_context->regs[6] = context->uc_mcontext.gregs[REG_RBP];
  save_context->regs[7] = context->uc_mcontext.gregs[REG_R8];
  save_context->regs[8] = context->uc_mcontext.gregs[REG_R9];
  save_context->regs[9] = context->uc_mcontext.gregs[REG_R10];
  save_context->regs[10] = context->uc_mcontext.gregs[REG_R11];
  save_context->regs[11] = context->uc_mcontext.gregs[REG_R12];
  save_context->regs[12] = context->uc_mcontext.gregs[REG_R13];
  save_context->regs[13] = context->uc_mcontext.gregs[REG_R14];
  save_context->regs[14] = context->uc_mcontext.gregs[REG_R15];
  save_context->fregs[0] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[0]);
  save_context->fregs[1] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[1]);
  save_context->fregs[2] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[2]);
  save_context->fregs[3] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[3]);
  save_context->fregs[4] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[4]);
  save_context->fregs[5] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[5]);
  save_context->fregs[6] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[6]);
  save_context->fregs[7] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[7]);
  save_context->fregs[8] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[8]);
  save_context->fregs[9] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[9]);
  save_context->fregs[10] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[10]);
  save_context->fregs[11] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[11]);
  save_context->fregs[12] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[12]);
  save_context->fregs[13] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[13]);
  save_context->fregs[14] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[14]);
  save_context->fregs[15] = xmm_low_bits(&context->uc_mcontext.fpregs->_xmm[15]);
}

//Write the given attributes to the processor context.
void load_sigaction_context (void* input_context,
                             uint64_t stack_pointer,
                             uint64_t arg0,
                             uint64_t arg1,
                             uint64_t instruction_pointer){
  ucontext_t* context = (ucontext_t*)input_context;
  context->uc_mcontext.gregs[REG_RSP] = stack_pointer;
  context->uc_mcontext.gregs[REG_RCX] = arg0;
  context->uc_mcontext.gregs[REG_RDX] = arg1;
  context->uc_mcontext.gregs[REG_RIP] = instruction_pointer;
}

#endif

//============================================================
//============================================================
//============================================================

extern uint64_t stanza_saved_c_rsp;
extern uint64_t stanza_stack_pointer;

//Global Stanza label where signal handler context is saved.
extern struct sigaction_context stanza_sighandler_context;

//Trampoline code for restoring execution context from
//the saved sigaction_context;
extern void stanza_sighandler_trampoline();

extern VMState stanza_vmstate;

uint64_t stanza_sighandler_trampoline_stack;

void* untag_ref (uint64_t ref){
  return (void*)(ref - 1 + 8);
}

//Call this from within the Stanza signal handler.
//Returns the address of the breakpoint that was hit.
uint64_t get_signal_handler_ip (){
  //The stored RIP is the address immediately after
  //the INT3 instruction, which is guaranteed to be 1-byte.
  //Therefore we subtract 1 to obtain the address of the breakpoint.
  return stanza_sighandler_context.rip - 1;
}

//Call this from within the Stanza signal handler.
//Returns the stack pointer of the child program at the time
//the INT3 was hit.
uint64_t get_signal_handler_sp (){
  return stanza_sighandler_context.rsp;
}

//1) Set Stanza saved_c_rsp to point to Stanza signal handling stack.
//2) Set RSP to top of Stanza's currently active stack.
//3) Retrieve the current signal_handler and jump to it using the Stanza calling convention.
void signal_handler (int signal, siginfo_t* info, void* input_context){
  save_sigaction_context(&stanza_sighandler_context, input_context, stanza_saved_c_rsp);

  //Set the return address to be the restore_sighandler_trampoline.
  *((uint64_t*)stanza_stack_pointer) = (uint64_t)stanza_sighandler_trampoline;

  //Set CRSP to point to signal handling stack.
  stanza_saved_c_rsp = stanza_sighandler_trampoline_stack;

  //Retrieve the sig-handler closure.
  uint64_t closure_ref = stanza_vmstate.signal_handler;
  Function* sighandler = untag_ref(closure_ref);

  //Load the context for execution:
  //- Set RSP to top of Stanza's currently active stack.
  //- Set Arg0 to closure.
  //- Set Arg1 to 0 (num arguments to closure).
  //- Set instruction pointer to closure starting address.
  load_sigaction_context(input_context,
                         stanza_stack_pointer,
                         closure_ref,
                         0,
                         sighandler->code);
}

void install_stanza_signal_interceptor () {
  //Allocate signal handling stack.
  uint64_t stackmem = (uint64_t)malloc(SIGSTKSZ);
  stanza_sighandler_trampoline_stack = stackmem + SIGSTKSZ - 8;

  //Instruct C to handle signals using
  //the separate signal handling stack.
  stack_t ss;
  ss.ss_sp = (void*)stackmem;
  ss.ss_size = SIGSTKSZ;
  ss.ss_flags = 0;
  if (sigaltstack(&ss, NULL) == -1){
    fprintf(stderr, "Failed to install signal interceptor.\n");
    perror("sigaltstack");
    exit(-1);
  }

  //Create signal action
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = signal_handler;
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigaction(SIGTRAP, &sa, NULL);
}
