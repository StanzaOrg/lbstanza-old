#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>

//============================================================
//=================== OPCODES ================================
//============================================================

#define SET_OPCODE_LOCAL 0
#define SET_OPCODE_UNSIGNED 1
#define SET_OPCODE_SIGNED 2
#define SET_OPCODE_CODE 3
#define SET_OPCODE_EXTERN 4
#define SET_OPCODE_GLOBAL 5
#define SET_OPCODE_DATA 6
#define SET_OPCODE_CONST 7
#define SET_OPCODE_WIDE 8
#define SET_REG_OPCODE_LOCAL 9
#define SET_REG_OPCODE_UNSIGNED 10
#define SET_REG_OPCODE_SIGNED 11
#define SET_REG_OPCODE_CODE 12
#define SET_REG_OPCODE_EXTERN 13
#define SET_REG_OPCODE_GLOBAL 14
#define SET_REG_OPCODE_DATA 15
#define SET_REG_OPCODE_CONST 16
#define SET_REG_OPCODE_WIDE 17
#define GET_REG_OPCODE 18
#define CALL_OPCODE_LOCAL 19
#define CALL_OPCODE_CODE 20
#define CALL_OPCODE_EXTERN 21
#define CALL_CLOSURE_OPCODE 22
#define TCALL_OPCODE_LOCAL 23
#define TCALL_OPCODE_CODE 24
#define TCALL_OPCODE_EXTERN 25
#define TCALL_CLOSURE_OPCODE 26
#define CALLC_OPCODE_LOCAL 27
#define CALLC_OPCODE_CODE 28
#define CALLC_OPCODE_EXTERN 29
#define POP_FRAME_OPCODE 30
#define LIVE_OPCODE 31
#define YIELD_OPCODE 32
#define RETURN_OPCODE 33
#define DUMP_OPCODE 34
#define INT_ADD_OPCODE 35
#define INT_SUB_OPCODE 36
#define INT_MUL_OPCODE 37
#define INT_DIV_OPCODE 38
#define INT_MOD_OPCODE 39
#define INT_AND_OPCODE 40
#define INT_OR_OPCODE 41
#define INT_XOR_OPCODE 42
#define INT_SHL_OPCODE 43
#define INT_SHR_OPCODE 44
#define INT_ASHR_OPCODE 45
#define INT_LT_OPCODE 46
#define INT_GT_OPCODE 47
#define INT_LE_OPCODE 48
#define INT_GE_OPCODE 49
#define EQ_OPCODE_REF_REF 50
#define EQ_OPCODE_REF 51
#define EQ_OPCODE_BYTE 52
#define EQ_OPCODE_INT 53
#define EQ_OPCODE_LONG 54
#define EQ_OPCODE_FLOAT 55
#define EQ_OPCODE_DOUBLE 56
#define NE_OPCODE_REF_REF 57
#define NE_OPCODE_REF 58
#define NE_OPCODE_BYTE 59
#define NE_OPCODE_INT 60
#define NE_OPCODE_LONG 61
#define NE_OPCODE_FLOAT 62
#define NE_OPCODE_DOUBLE 63
#define ADD_OPCODE_BYTE 64
#define ADD_OPCODE_INT 65
#define ADD_OPCODE_LONG 66
#define ADD_OPCODE_FLOAT 67
#define ADD_OPCODE_DOUBLE 68
#define SUB_OPCODE_BYTE 69
#define SUB_OPCODE_INT 70
#define SUB_OPCODE_LONG 71
#define SUB_OPCODE_FLOAT 72
#define SUB_OPCODE_DOUBLE 73
#define MUL_OPCODE_BYTE 74
#define MUL_OPCODE_INT 75
#define MUL_OPCODE_LONG 76
#define MUL_OPCODE_FLOAT 77
#define MUL_OPCODE_DOUBLE 78
#define DIV_OPCODE_BYTE 79
#define DIV_OPCODE_INT 80
#define DIV_OPCODE_LONG 81
#define DIV_OPCODE_FLOAT 82
#define DIV_OPCODE_DOUBLE 83
#define MOD_OPCODE_BYTE 84
#define MOD_OPCODE_INT 85
#define MOD_OPCODE_LONG 86
#define AND_OPCODE_BYTE 87
#define AND_OPCODE_INT 88
#define AND_OPCODE_LONG 89
#define OR_OPCODE_BYTE 90
#define OR_OPCODE_INT 91
#define OR_OPCODE_LONG 92
#define XOR_OPCODE_BYTE 93
#define XOR_OPCODE_INT 94
#define XOR_OPCODE_LONG 95
#define SHL_OPCODE_BYTE 96
#define SHL_OPCODE_INT 97
#define SHL_OPCODE_LONG 98
#define SHR_OPCODE_BYTE 99
#define SHR_OPCODE_INT 100
#define SHR_OPCODE_LONG 101
#define ASHR_OPCODE_BYTE 102
#define ASHR_OPCODE_INT 103
#define ASHR_OPCODE_LONG 104
#define LT_OPCODE_INT 105
#define LT_OPCODE_LONG 106
#define LT_OPCODE_FLOAT 107
#define LT_OPCODE_DOUBLE 108
#define GT_OPCODE_INT 109
#define GT_OPCODE_LONG 110
#define GT_OPCODE_FLOAT 111
#define GT_OPCODE_DOUBLE 112
#define LE_OPCODE_INT 113
#define LE_OPCODE_LONG 114
#define LE_OPCODE_FLOAT 115
#define LE_OPCODE_DOUBLE 116
#define GE_OPCODE_INT 117
#define GE_OPCODE_LONG 118
#define GE_OPCODE_FLOAT 119
#define GE_OPCODE_DOUBLE 120
#define ULE_OPCODE_BYTE 121
#define ULE_OPCODE_INT 122
#define ULE_OPCODE_LONG 123
#define ULT_OPCODE_BYTE 124
#define ULT_OPCODE_INT 125
#define ULT_OPCODE_LONG 126
#define UGT_OPCODE_BYTE 127
#define UGT_OPCODE_INT 128
#define UGT_OPCODE_LONG 129
#define UGE_OPCODE_BYTE 130
#define UGE_OPCODE_INT 131
#define UGE_OPCODE_LONG 132
#define INT_NOT_OPCODE 133
#define INT_NEG_OPCODE 134
#define NOT_OPCODE_BYTE 135
#define NOT_OPCODE_INT 136
#define NOT_OPCODE_LONG 137
#define NEG_OPCODE_INT 138
#define NEG_OPCODE_LONG 139
#define NEG_OPCODE_FLOAT 140
#define NEG_OPCODE_DOUBLE 141
#define DEREF_OPCODE 142
#define TYPEOF_OPCODE 143
#define JUMP_SET_OPCODE 144
#define GOTO_OPCODE 145
#define CONV_OPCODE_BYTE_FLOAT 146
#define CONV_OPCODE_BYTE_DOUBLE 147
#define CONV_OPCODE_INT_BYTE 148
#define CONV_OPCODE_INT_FLOAT 149
#define CONV_OPCODE_INT_DOUBLE 150
#define CONV_OPCODE_LONG_BYTE 151
#define CONV_OPCODE_LONG_INT 152
#define CONV_OPCODE_LONG_FLOAT 153
#define CONV_OPCODE_LONG_DOUBLE 154
#define CONV_OPCODE_FLOAT_BYTE 155
#define CONV_OPCODE_FLOAT_INT 156
#define CONV_OPCODE_FLOAT_LONG 157
#define CONV_OPCODE_FLOAT_DOUBLE 158
#define CONV_OPCODE_DOUBLE_BYTE 159
#define CONV_OPCODE_DOUBLE_INT 160
#define CONV_OPCODE_DOUBLE_LONG 161
#define CONV_OPCODE_DOUBLE_FLOAT 162
#define DETAG_OPCODE 163
#define TAG_OPCODE_BYTE 164
#define TAG_OPCODE_CHAR 165
#define TAG_OPCODE_INT 166
#define TAG_OPCODE_FLOAT 167
#define STORE_OPCODE_1 168
#define STORE_OPCODE_4 169
#define STORE_OPCODE_8 170
#define STORE_OPCODE_1_VAR_OFFSET 171
#define STORE_OPCODE_4_VAR_OFFSET 172
#define STORE_OPCODE_8_VAR_OFFSET 173
#define LOAD_OPCODE_1 174
#define LOAD_OPCODE_4 175
#define LOAD_OPCODE_8 176
#define LOAD_OPCODE_1_VAR_OFFSET 177
#define LOAD_OPCODE_4_VAR_OFFSET 178
#define LOAD_OPCODE_8_VAR_OFFSET 179
#define RESERVE_OPCODE_LOCAL 180
#define RESERVE_OPCODE_CONST 181
#define NEW_STACK_OPCODE 182
#define ALLOC_OPCODE_CONST 183
#define ALLOC_OPCODE_LOCAL 184
#define GC_OPCODE 185
#define PRINT_STACK_TRACE_OPCODE 186
#define CURRENT_STACK_OPCODE 187
#define FLUSH_VM_OPCODE 188
#define GLOBALS_OPCODE 189
#define CONSTS_OPCODE 190
#define CONSTS_DATA_OPCODE 191
#define JUMP_INT_LT_OPCODE 192
#define JUMP_INT_GT_OPCODE 193
#define JUMP_INT_LE_OPCODE 194
#define JUMP_INT_GE_OPCODE 195
#define JUMP_EQ_OPCODE_REF 196
#define JUMP_EQ_OPCODE_BYTE 197
#define JUMP_EQ_OPCODE_INT 198
#define JUMP_EQ_OPCODE_LONG 199
#define JUMP_EQ_OPCODE_FLOAT 200
#define JUMP_EQ_OPCODE_DOUBLE 201
#define JUMP_NE_OPCODE_REF 202
#define JUMP_NE_OPCODE_BYTE 203
#define JUMP_NE_OPCODE_INT 204
#define JUMP_NE_OPCODE_LONG 205
#define JUMP_NE_OPCODE_FLOAT 206
#define JUMP_NE_OPCODE_DOUBLE 207
#define JUMP_LT_OPCODE_INT 208
#define JUMP_LT_OPCODE_LONG 209
#define JUMP_LT_OPCODE_FLOAT 210
#define JUMP_LT_OPCODE_DOUBLE 211
#define JUMP_GT_OPCODE_INT 212
#define JUMP_GT_OPCODE_LONG 213
#define JUMP_GT_OPCODE_FLOAT 214
#define JUMP_GT_OPCODE_DOUBLE 215
#define JUMP_LE_OPCODE_INT 216
#define JUMP_LE_OPCODE_LONG 217
#define JUMP_LE_OPCODE_FLOAT 218
#define JUMP_LE_OPCODE_DOUBLE 219
#define JUMP_GE_OPCODE_INT 220
#define JUMP_GE_OPCODE_LONG 221
#define JUMP_GE_OPCODE_FLOAT 222
#define JUMP_GE_OPCODE_DOUBLE 223
#define JUMP_ULE_OPCODE_BYTE 224
#define JUMP_ULE_OPCODE_INT 225
#define JUMP_ULE_OPCODE_LONG 226
#define JUMP_ULT_OPCODE_BYTE 227
#define JUMP_ULT_OPCODE_INT 228
#define JUMP_ULT_OPCODE_LONG 229
#define JUMP_UGT_OPCODE_BYTE 230
#define JUMP_UGT_OPCODE_INT 231
#define JUMP_UGT_OPCODE_LONG 232
#define JUMP_UGE_OPCODE_BYTE 233
#define JUMP_UGE_OPCODE_INT 234
#define JUMP_UGE_OPCODE_LONG 235
#define DISPATCH_OPCODE 236
#define DISPATCH_METHOD_OPCODE 237
#define JUMP_REG_OPCODE 238

//============================================================
//===================== READ MACROS ==========================
//============================================================

#define PC_INT() \
  ({unsigned int _x = *(unsigned int*)pc; \
    pc += 4; \
    _x;});

#define PC_LONG() \
  ({uint64_t _x = *(uint64_t*)pc; \
    pc += 8; \
    _x;});

//Instruction sizes
#define ASIZE 4
#define BSIZE 4
#define CSIZE 8
#define DSIZE 12
#define ESIZE 8
#define FSIZE 8

#define DECODE_A_UNSIGNED() \
  int value = W1 >> 8; \
  printf("[%d | %d]\n", opcode, value);

#define DECODE_A_SIGNED() \
  int value = (int)W1 >> 8; \
  printf("[%d | %d]\n", opcode, value);

#define DECODE_B_UNSIGNED() \
  int x = (W1 >> 8) & 0x3FF; \
  int value = W1 >> 18; \
  printf("[%d | %d | %d]\n", opcode, x, value);

#define DECODE_C() \
  int x = (W1 >> 8) & 0x3FF; \
  int y = (W1 >> 22) & 0x3FF; \
  int value = PC_INT(); \
  printf("[%d | %d | %d | %d]\n", opcode, x, y, value);

#define DECODE_D() \
  int x = (W1 >> 8) & 0x3FF; \
  int y = (W1 >> 22) & 0x3FF; \
  long value = PC_LONG(); \
  printf("[%d | %d | %d | %ld]\n", opcode, x, y, value);

#define DECODE_E() \
  unsigned int W2 = PC_INT(); \
  uint64_t W12 = W1 | ((uint64_t)W2 << 32); \
  int x = (int)(W12 >> 8) & 0x3FF;  \
  int y = (int)(W12 >> 18) & 0x3FF; \
  int z = (int)(W12 >> 28) & 0x3FF; \
  int value = (int)((int64_t)W12 >> 38); \
  printf("[%d | %d | %d | %d | %d]\n", opcode, x, y, z, value);

#define DECODE_F() \
  unsigned int W2 = PC_INT(); \
  uint64_t W12 = W1 | ((uint64_t)W2 << 32); \
  int x = (int)(W12 >> 8) & 0x3FF;  \
  int y = (int)(W12 >> 18) & 0x3FF; \
  int _n1 = (int)(W12 >> 14); /*Move first bit to 32-bit boundary*/ \
  int n1 = (int)(_n1 >> 14); /*Extend sign-bit*/ \
  int n2 = (int)((int)W2 >> 14); /*Extend sign-bit of first word*/ \
  printf("[%d | %d | %d | %d | %d]\n", opcode, x, y, n1, n2);

#define DECODE_TGTS() \
  int n = PC_INT(); \
  for(int i=0; i<n; i++){ \
    int tgt = PC_INT(); \
    printf("  tgt: %d\n", tgt); \
  }

#define SET_REG(r,v) \
  registers[r] = v    
#define SET_LOCAL(l,v) \
  stack_pointer->slots[l] = v;
#define LOCAL(l) (stack_pointer->slots[l])

//============================================================
//========================= TRAPS ============================
//============================================================

int call_c_launcher (int index, uint64_t faddr, uint64_t* registers);

//============================================================
//===================== MAIN LOOP ============================
//============================================================

//What does the machine need to run?
//- Pointer to instructions
//- Pointer to registers
//- Pointer to top of heap (for allocation)
//- Pointer to limit of heap (for knowing when to call gc)
//- Pointer to stack (for call frames)
//- Pointer to end of stack (for knowing when to build stack)
//- Current stack

typedef struct{
  uint64_t returnpc;
  uint64_t liveness_map;
  uint64_t slots[];
} StackFrame;

typedef struct{
  uint64_t size;
  StackFrame* frames;
  StackFrame* stack_pointer;
  int pc;
} Stack;

Stack* untag_stack (uint64_t current_stack){
  return (Stack*)(current_stack - 1 + 8);
}

uint64_t ptr_to_ref (void* p){
  return (uint64_t)p + 1;
}

void vmloop (char* instructions, int n,
             char* heap_top,
             char* heap_limit,
             uint64_t* registers,
             uint64_t current_stack,
             uint64_t* global_offsets,
             char* global_mem,
             uint64_t* consts_table,
             uint64_t* new_consts,
             char* consts_data_mem,
             uint64_t* data_offsets,
             char* data_mem,
             uint64_t* extern_table,
             uint32_t* code_offsets){
  printf("VM Loop!\n");
  printf("Instructions = %p\n", instructions);
  printf("Total = %d bytes\n", n);
  printf("heap_top = %p\n", heap_top);
  printf("heap_limit = %p\n", heap_limit);
  printf("heap_registers = %p\n", registers);
  printf("current_stack = %llx\n", current_stack);

  //Retrieve starting program counter
  Stack* stk = untag_stack(current_stack);
  StackFrame* stack_pointer = stk->stack_pointer;
  char* pc = instructions + stk->pc * 4;

  //Machine Parameters
  //char* pc = instructions;
  //char* pc_end = instructions+n;
  //while(pc < pc_end){
  while(1){
    unsigned int W1 = PC_INT();
    int opcode = W1 & 0xFF;
    switch(opcode){
    case SET_OPCODE_LOCAL : {
      DECODE_C();
      SET_LOCAL(y, LOCAL(value));      
      continue;
    }
    case SET_OPCODE_UNSIGNED : {
      DECODE_C();
      SET_LOCAL(y, (uint64_t)value);      
      continue;
    }
    case SET_OPCODE_SIGNED : {
      DECODE_C();
      SET_LOCAL(y, (int64_t)value);      
      continue;
    }
    case SET_OPCODE_CODE : {
      DECODE_C();
      SET_LOCAL(y, value);
      continue;
    }
    case SET_OPCODE_EXTERN : {
      DECODE_C();
      SET_LOCAL(y, extern_table[value]);
      continue;
    }
    case SET_OPCODE_GLOBAL : {
      DECODE_C();
      char* address = global_mem + global_offsets[value];
      SET_LOCAL(y, (uint64_t)address);
      continue;
    }
    case SET_OPCODE_DATA : {
      DECODE_C();
      char* address = data_mem + data_offsets[value];
      SET_LOCAL(y, (uint64_t)address);
      continue;
    }
    case SET_OPCODE_CONST : {
      DECODE_C();
      SET_LOCAL(y, consts_table[value]);
      continue;
    }
    case SET_OPCODE_WIDE : {
      DECODE_D();
      SET_LOCAL(y, value);      
      continue;
    }
    case SET_REG_OPCODE_LOCAL : {
      DECODE_C();
      SET_REG(y, LOCAL(value));      
      continue;
    }
    case SET_REG_OPCODE_UNSIGNED : {
      DECODE_C();
      SET_REG(y, (uint64_t)value);   
      continue;
    }
    case SET_REG_OPCODE_SIGNED : {
      DECODE_C();
      SET_REG(y, (int64_t)value); 
      continue;
    }
    case SET_REG_OPCODE_CODE : {
      DECODE_C();
      SET_REG(y, value);
      continue;
    }
    case SET_REG_OPCODE_EXTERN : {
      DECODE_C();
      SET_REG(y, extern_table[value]);
      continue;
    }
    case SET_REG_OPCODE_GLOBAL : {
      DECODE_C();
      char* address = global_mem + global_offsets[value];
      SET_REG(y, (uint64_t)address);
      continue;
    }
    case SET_REG_OPCODE_DATA : {
      DECODE_C();
      char* address = data_mem + data_offsets[value];
      SET_REG(y, (uint64_t)address);
      continue;
    }
    case SET_REG_OPCODE_CONST : {
      DECODE_C();
      SET_REG(y, consts_table[value]);
      continue;
    }
    case SET_REG_OPCODE_WIDE : {
      DECODE_D();
      SET_REG(y, value);      
      continue;
    }
    case GET_REG_OPCODE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CALL_OPCODE_LOCAL : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CALL_OPCODE_CODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CALL_OPCODE_EXTERN : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CALL_CLOSURE_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case TCALL_OPCODE_LOCAL : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case TCALL_OPCODE_CODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case TCALL_OPCODE_EXTERN : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case TCALL_CLOSURE_OPCODE : {
      DECODE_A_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CALLC_OPCODE_LOCAL : {
      DECODE_C();
      uint64_t faddr = LOCAL(value);
      int format = x;
      int num_locals = y;
      stack_pointer = (StackFrame*)((char*)stack_pointer + num_locals * 8);
      call_c_launcher(format, faddr, registers);
      stack_pointer = (StackFrame*)((char*)stack_pointer - num_locals * 8);
      continue;
    }
    case CALLC_OPCODE_CODE : {
      DECODE_C();
      int offset = code_offsets[value];
      uint64_t faddr = (uint64_t)(instructions + offset * 4);
      int format = x;
      int num_locals = y;
      stack_pointer = (StackFrame*)((char*)stack_pointer + num_locals * 8);
      call_c_launcher(format, faddr, registers);
      stack_pointer = (StackFrame*)((char*)stack_pointer - num_locals * 8);
      continue;
    }
    case CALLC_OPCODE_EXTERN : {
      DECODE_C();
      uint64_t faddr = extern_table[value];
      int format = x;
      int num_locals = y;
      stack_pointer = (StackFrame*)((char*)stack_pointer + num_locals * 8);
      call_c_launcher(format, faddr, registers);
      stack_pointer = (StackFrame*)((char*)stack_pointer - num_locals * 8);
      continue;
    }
    case POP_FRAME_OPCODE : {
      DECODE_A_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LIVE_OPCODE : {
      DECODE_A_UNSIGNED();
      stack_pointer->liveness_map = value;
      continue;
    }
    case YIELD_OPCODE : {
      DECODE_A_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case RETURN_OPCODE : {
      DECODE_A_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case DUMP_OPCODE : {
      DECODE_A_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_ADD_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_SUB_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_MUL_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_DIV_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_MOD_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_AND_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_OR_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_XOR_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_SHL_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_SHR_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_ASHR_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_LT_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_GT_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_LE_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_GE_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case EQ_OPCODE_REF_REF : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case EQ_OPCODE_REF : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case EQ_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case EQ_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case EQ_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case EQ_OPCODE_FLOAT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case EQ_OPCODE_DOUBLE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NE_OPCODE_REF_REF : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NE_OPCODE_REF : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NE_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NE_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NE_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NE_OPCODE_FLOAT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NE_OPCODE_DOUBLE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ADD_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ADD_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ADD_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ADD_OPCODE_FLOAT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ADD_OPCODE_DOUBLE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case SUB_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case SUB_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case SUB_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case SUB_OPCODE_FLOAT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case SUB_OPCODE_DOUBLE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case MUL_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case MUL_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case MUL_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case MUL_OPCODE_FLOAT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case MUL_OPCODE_DOUBLE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case DIV_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case DIV_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case DIV_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case DIV_OPCODE_FLOAT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case DIV_OPCODE_DOUBLE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case MOD_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case MOD_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case MOD_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case AND_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case AND_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case AND_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case OR_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case OR_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case OR_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case XOR_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case XOR_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case XOR_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case SHL_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case SHL_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case SHL_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case SHR_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case SHR_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case SHR_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ASHR_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ASHR_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ASHR_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LT_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LT_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LT_OPCODE_FLOAT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LT_OPCODE_DOUBLE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case GT_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case GT_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case GT_OPCODE_FLOAT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case GT_OPCODE_DOUBLE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LE_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LE_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LE_OPCODE_FLOAT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LE_OPCODE_DOUBLE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case GE_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case GE_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case GE_OPCODE_FLOAT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case GE_OPCODE_DOUBLE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ULE_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ULE_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ULE_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ULT_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ULT_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ULT_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case UGT_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case UGT_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case UGT_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case UGE_OPCODE_BYTE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case UGE_OPCODE_INT : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case UGE_OPCODE_LONG : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_NOT_OPCODE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case INT_NEG_OPCODE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NOT_OPCODE_BYTE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NOT_OPCODE_INT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NOT_OPCODE_LONG : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NEG_OPCODE_INT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NEG_OPCODE_LONG : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NEG_OPCODE_FLOAT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case NEG_OPCODE_DOUBLE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case DEREF_OPCODE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case TYPEOF_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_SET_OPCODE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case GOTO_OPCODE : {
      DECODE_A_SIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_BYTE_FLOAT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_BYTE_DOUBLE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_INT_BYTE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_INT_FLOAT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_INT_DOUBLE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_LONG_BYTE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_LONG_INT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_LONG_FLOAT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_LONG_DOUBLE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_FLOAT_BYTE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_FLOAT_INT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_FLOAT_LONG : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_FLOAT_DOUBLE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_DOUBLE_BYTE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_DOUBLE_INT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_DOUBLE_LONG : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONV_OPCODE_DOUBLE_FLOAT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case DETAG_OPCODE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case TAG_OPCODE_BYTE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case TAG_OPCODE_CHAR : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case TAG_OPCODE_INT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case TAG_OPCODE_FLOAT : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case STORE_OPCODE_1 : {
      DECODE_E();
      char* address = (char*)(LOCAL(x) + value);
      char storeval = (char)(LOCAL(z));
      *address = storeval;
      continue;
    }
    case STORE_OPCODE_4 : {
      DECODE_E();
      int32_t* address = (int32_t*)(LOCAL(x) + value);
      int32_t storeval = (int32_t)(LOCAL(z));
      *address = storeval;     
      continue;
    }
    case STORE_OPCODE_8 : {
      DECODE_E();
      int64_t* address = (int64_t*)(LOCAL(x) + value);
      int64_t storeval = (int64_t)(LOCAL(z));
      *address = storeval;     
      continue;
    }
    case STORE_OPCODE_1_VAR_OFFSET : {
      DECODE_E();
      char* address = (char*)(LOCAL(x) + LOCAL(y) + value);
      char storeval = (char)(LOCAL(z));
      *address = storeval;
      continue;
    }
    case STORE_OPCODE_4_VAR_OFFSET : {
      DECODE_E();
      int32_t* address = (int32_t*)(LOCAL(x) + LOCAL(y) + value);
      int32_t storeval = (int32_t)(LOCAL(z));
      *address = storeval;
      continue;
    }
    case STORE_OPCODE_8_VAR_OFFSET : {
      DECODE_E();
      int64_t* address = (int64_t*)(LOCAL(x) + LOCAL(y) + value);
      int64_t storeval = (int64_t)(LOCAL(z));
      *address = storeval;     
      continue;
    }
    case LOAD_OPCODE_1 : {
      DECODE_E();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LOAD_OPCODE_4 : {
      DECODE_E();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LOAD_OPCODE_8 : {
      DECODE_E();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LOAD_OPCODE_1_VAR_OFFSET : {
      DECODE_E();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LOAD_OPCODE_4_VAR_OFFSET : {
      DECODE_E();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case LOAD_OPCODE_8_VAR_OFFSET : {
      DECODE_E();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case RESERVE_OPCODE_LOCAL : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case RESERVE_OPCODE_CONST : {
      DECODE_C();
      int size = value;
      int offset = x * 4;
      if(heap_top + size <= heap_limit){
        pc += offset - CSIZE;
        continue;
      }else{
        printf("GC Not yet implemented.\n");
        exit(-1);
      }
    }
    case NEW_STACK_OPCODE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case ALLOC_OPCODE_CONST : {
      DECODE_C();
      int num_bytes = y;
      int type = value;
      *(uint64_t*)heap_top = type;
      SET_LOCAL(x, ptr_to_ref(heap_top));
      heap_top = heap_top + num_bytes;
      continue;
    }
    case ALLOC_OPCODE_LOCAL : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case GC_OPCODE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case PRINT_STACK_TRACE_OPCODE : {
      DECODE_B_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CURRENT_STACK_OPCODE : {
      DECODE_A_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case FLUSH_VM_OPCODE : {
      DECODE_A_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case GLOBALS_OPCODE : {
      DECODE_A_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONSTS_OPCODE : {
      DECODE_A_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case CONSTS_DATA_OPCODE : {
      DECODE_A_UNSIGNED();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_INT_LT_OPCODE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_INT_GT_OPCODE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_INT_LE_OPCODE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_INT_GE_OPCODE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_EQ_OPCODE_REF : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_EQ_OPCODE_BYTE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_EQ_OPCODE_INT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_EQ_OPCODE_LONG : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_EQ_OPCODE_FLOAT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_EQ_OPCODE_DOUBLE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_NE_OPCODE_REF : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_NE_OPCODE_BYTE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_NE_OPCODE_INT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_NE_OPCODE_LONG : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_NE_OPCODE_FLOAT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_NE_OPCODE_DOUBLE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_LT_OPCODE_INT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_LT_OPCODE_LONG : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_LT_OPCODE_FLOAT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_LT_OPCODE_DOUBLE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_GT_OPCODE_INT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_GT_OPCODE_LONG : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_GT_OPCODE_FLOAT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_GT_OPCODE_DOUBLE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_LE_OPCODE_INT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_LE_OPCODE_LONG : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_LE_OPCODE_FLOAT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_LE_OPCODE_DOUBLE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_GE_OPCODE_INT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_GE_OPCODE_LONG : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_GE_OPCODE_FLOAT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_GE_OPCODE_DOUBLE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_ULE_OPCODE_BYTE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_ULE_OPCODE_INT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_ULE_OPCODE_LONG : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_ULT_OPCODE_BYTE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_ULT_OPCODE_INT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_ULT_OPCODE_LONG : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_UGT_OPCODE_BYTE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_UGT_OPCODE_INT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_UGT_OPCODE_LONG : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_UGE_OPCODE_BYTE : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_UGE_OPCODE_INT : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_UGE_OPCODE_LONG : {
      DECODE_F();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case DISPATCH_OPCODE : {
      DECODE_A_UNSIGNED();
      DECODE_TGTS();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case DISPATCH_METHOD_OPCODE : {
      DECODE_A_UNSIGNED();
      DECODE_TGTS();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    case JUMP_REG_OPCODE : {
      DECODE_C();
      printf("Not yet implemented.\n");
      exit(-1);
      continue;
    }
    }

    //Done
    printf("opcode = %d\n", opcode);
    printf("W1 = %x\n", W1);
    exit(-1);
    return;
  }
}
