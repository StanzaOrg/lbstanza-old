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
//============================================================
//============================================================

extern uint64_t stanza_saved_c_rsp;
extern uint64_t stanza_stack_pointer;

//Saved context for the sigaction_handler.
struct sigaction_context{
  uint64_t rip;
  uint64_t rsp;
  uint64_t crsp;
  uint64_t regs[15];
  uint64_t fregs[16];
};
extern struct sigaction_context stanza_sighandler_context;

//Trampoline code for restoring execution context from
//the saved sigaction_context;
extern void stanza_sighandler_trampoline();

extern VMState stanza_vmstate;

uint64_t stanza_sighandler_trampoline_stack;

void* untag_ref (uint64_t ref){
  return (void*)(ref - 1 + 8);
}

uint64_t xmm_low_bits (struct __darwin_xmm_reg* xmm){
  uint64_t* p = (uint64_t*)xmm;
  return p[1];
}

//1) Set Stanza saved_c_rsp to point to Stanza signal handling stack.
//2) Set RSP to top of Stanza's currently active stack.
//3) Retrieve the current signal_handler and jump to it using the Stanza calling convention.
void signal_handler (int signal, siginfo_t* info, void* input_context){
  ucontext_t* context = (ucontext_t*)input_context;

  //Save the signal handler
  stanza_sighandler_context.rip = context->uc_mcontext->__ss.__rip;
  stanza_sighandler_context.rsp = context->uc_mcontext->__ss.__rsp;
  stanza_sighandler_context.crsp = stanza_saved_c_rsp;
  stanza_sighandler_context.regs[0] = context->uc_mcontext->__ss.__rax;
  stanza_sighandler_context.regs[1] = context->uc_mcontext->__ss.__rbx;
  stanza_sighandler_context.regs[2] = context->uc_mcontext->__ss.__rcx;
  stanza_sighandler_context.regs[3] = context->uc_mcontext->__ss.__rdx;
  stanza_sighandler_context.regs[4] = context->uc_mcontext->__ss.__rsi;
  stanza_sighandler_context.regs[5] = context->uc_mcontext->__ss.__rdi;
  stanza_sighandler_context.regs[6] = context->uc_mcontext->__ss.__rbp;
  stanza_sighandler_context.regs[7] = context->uc_mcontext->__ss.__r8;
  stanza_sighandler_context.regs[8] = context->uc_mcontext->__ss.__r9;
  stanza_sighandler_context.regs[9] = context->uc_mcontext->__ss.__r10;
  stanza_sighandler_context.regs[10] = context->uc_mcontext->__ss.__r11;
  stanza_sighandler_context.regs[11] = context->uc_mcontext->__ss.__r12;
  stanza_sighandler_context.regs[12] = context->uc_mcontext->__ss.__r13;
  stanza_sighandler_context.regs[13] = context->uc_mcontext->__ss.__r14;
  stanza_sighandler_context.regs[14] = context->uc_mcontext->__ss.__r15;
  stanza_sighandler_context.fregs[0] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm0);
  stanza_sighandler_context.fregs[1] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm1);
  stanza_sighandler_context.fregs[2] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm2);
  stanza_sighandler_context.fregs[3] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm3);
  stanza_sighandler_context.fregs[4] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm4);
  stanza_sighandler_context.fregs[5] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm5);
  stanza_sighandler_context.fregs[6] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm6);
  stanza_sighandler_context.fregs[7] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm7);
  stanza_sighandler_context.fregs[8] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm8);
  stanza_sighandler_context.fregs[9] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm9);
  stanza_sighandler_context.fregs[10] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm10);
  stanza_sighandler_context.fregs[11] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm11);
  stanza_sighandler_context.fregs[12] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm12);
  stanza_sighandler_context.fregs[13] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm13);
  stanza_sighandler_context.fregs[14] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm14);
  stanza_sighandler_context.fregs[15] = xmm_low_bits(&context->uc_mcontext->__fs.__fpu_xmm15);

  //Set RSP to top of Stanza's currently active stack.
  context->uc_mcontext->__ss.__rsp = stanza_stack_pointer;

  //Set the return address to be the restore_sighandler_trampoline.
  *((uint64_t*)stanza_stack_pointer) = (uint64_t)stanza_sighandler_trampoline;

  //Set CRSP to point to signal handling stack.  
  stanza_saved_c_rsp = stanza_sighandler_trampoline_stack;

  //Retrieve the sig-handler closure.
  uint64_t closure_ref = stanza_vmstate.signal_handler;
  Function* sighandler = untag_ref(closure_ref);

  //Write the arguments to the closure call.
  //Arg0 (RCX) = closure
  //Arg1 (RDX) = num arguments = 0
  context->uc_mcontext->__ss.__rcx = closure_ref;
  context->uc_mcontext->__ss.__rdx = 0;
  
  //Jump to the closure starting address.
  context->uc_mcontext->__ss.__rip = sighandler->code;  
}

void install_stanza_signal_interceptor () {
  //Allocate signal handling stack.
  uint64_t stackmem = (uint64_t)malloc(SIGSTKSZ);
  stanza_sighandler_trampoline_stack = stackmem + SIGSTKSZ - 8;

  //Instruct C to handle signals using
  //the separate signal handling stack.
  stack_t ss;
  ss.ss_sp = stackmem;
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
