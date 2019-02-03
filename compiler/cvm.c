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
#define STORE_OPCODE 168
#define STORE_OPCODE_VAR_OFFSET 169
#define STORE_OPCODE_REF 170
#define STORE_OPCODE_REF_VAR_OFFSET 171
#define LOAD_OPCODE 172
#define LOAD_OPCODE_VAR_OFFSET 173
#define LOAD_OPCODE_REF 174
#define LOAD_OPCODE_REF_VAR_OFFSET 175
#define RESERVE_OPCODE_LOCAL 176
#define RESERVE_OPCODE_CONST 177
#define NEW_STACK_OPCODE 178
#define ALLOC_OPCODE_CONST 179
#define ALLOC_OPCODE_LOCAL 180
#define GC_OPCODE 181
#define PRINT_STACK_TRACE_OPCODE 182
#define CURRENT_STACK_OPCODE 183
#define FLUSH_VM_OPCODE 184
#define GLOBALS_OPCODE 185
#define CONSTS_OPCODE 186
#define CONSTS_DATA_OPCODE 187
#define JUMP_INT_LT_OPCODE 188
#define JUMP_INT_GT_OPCODE 189
#define JUMP_INT_LE_OPCODE 190
#define JUMP_INT_GE_OPCODE 191
#define JUMP_EQ_OPCODE_REF 192
#define JUMP_EQ_OPCODE_BYTE 193
#define JUMP_EQ_OPCODE_INT 194
#define JUMP_EQ_OPCODE_LONG 195
#define JUMP_EQ_OPCODE_FLOAT 196
#define JUMP_EQ_OPCODE_DOUBLE 197
#define JUMP_NE_OPCODE_REF 198
#define JUMP_NE_OPCODE_BYTE 199
#define JUMP_NE_OPCODE_INT 200
#define JUMP_NE_OPCODE_LONG 201
#define JUMP_NE_OPCODE_FLOAT 202
#define JUMP_NE_OPCODE_DOUBLE 203
#define JUMP_LT_OPCODE_INT 204
#define JUMP_LT_OPCODE_LONG 205
#define JUMP_LT_OPCODE_FLOAT 206
#define JUMP_LT_OPCODE_DOUBLE 207
#define JUMP_GT_OPCODE_INT 208
#define JUMP_GT_OPCODE_LONG 209
#define JUMP_GT_OPCODE_FLOAT 210
#define JUMP_GT_OPCODE_DOUBLE 211
#define JUMP_LE_OPCODE_INT 212
#define JUMP_LE_OPCODE_LONG 213
#define JUMP_LE_OPCODE_FLOAT 214
#define JUMP_LE_OPCODE_DOUBLE 215
#define JUMP_GE_OPCODE_INT 216
#define JUMP_GE_OPCODE_LONG 217
#define JUMP_GE_OPCODE_FLOAT 218
#define JUMP_GE_OPCODE_DOUBLE 219
#define JUMP_ULE_OPCODE_BYTE 220
#define JUMP_ULE_OPCODE_INT 221
#define JUMP_ULE_OPCODE_LONG 222
#define JUMP_ULT_OPCODE_BYTE 223
#define JUMP_ULT_OPCODE_INT 224
#define JUMP_ULT_OPCODE_LONG 225
#define JUMP_UGT_OPCODE_BYTE 226
#define JUMP_UGT_OPCODE_INT 227
#define JUMP_UGT_OPCODE_LONG 228
#define JUMP_UGE_OPCODE_BYTE 229
#define JUMP_UGE_OPCODE_INT 230
#define JUMP_UGE_OPCODE_LONG 231
#define DISPATCH_OPCODE 232
#define DISPATCH_METHOD_OPCODE 233
#define JUMP_REG_OPCODE 234

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
    

//============================================================
//===================== MAIN LOOP ============================
//============================================================

void vmloop (char* instructions, int n){
  printf("VM Loop!\n");
  printf("Instructions = %p\n", instructions);
  printf("Total = %d bytes\n", n);

//  //Print out characters
//  for(int i=0; i<n; i+=4){
//    int* pc = (int*)(instructions + i);
//    int word = *pc;
//    printf("%d ", word);
//  }
//  printf("\n");


  //Machine Parameters
  char* pc = instructions;
  char* pc_end = instructions+n;
  while(pc < pc_end){
    unsigned int W1 = PC_INT();
    int opcode = W1 & 0xFF;
    switch(opcode){
    case SET_OPCODE_LOCAL : {
      DECODE_C();
      continue;
    }
    case SET_OPCODE_UNSIGNED : {
      DECODE_C();
      continue;
    }
    case SET_OPCODE_SIGNED : {
      DECODE_C();
      continue;
    }
    case SET_OPCODE_CODE : {
      DECODE_C();
      continue;
    }
    case SET_OPCODE_EXTERN : {
      DECODE_C();
      continue;
    }
    case SET_OPCODE_GLOBAL : {
      DECODE_C();
      continue;
    }
    case SET_OPCODE_DATA : {
      DECODE_C();
      continue;
    }
    case SET_OPCODE_CONST : {
      DECODE_C();
      continue;
    }
    case SET_OPCODE_WIDE : {
      DECODE_D();
      continue;
    }
    case SET_REG_OPCODE_LOCAL : {
      DECODE_C();
      continue;
    }
    case SET_REG_OPCODE_UNSIGNED : {
      DECODE_C();
      continue;
    }
    case SET_REG_OPCODE_SIGNED : {
      DECODE_C();
      continue;
    }
    case SET_REG_OPCODE_CODE : {
      DECODE_C();
      continue;
    }
    case SET_REG_OPCODE_EXTERN : {
      DECODE_C();
      continue;
    }
    case SET_REG_OPCODE_GLOBAL : {
      DECODE_C();
      continue;
    }
    case SET_REG_OPCODE_DATA : {
      DECODE_C();
      continue;
    }
    case SET_REG_OPCODE_CONST : {
      DECODE_C();
      continue;
    }
    case SET_REG_OPCODE_WIDE : {
      DECODE_D();
      continue;
    }
    case GET_REG_OPCODE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CALL_OPCODE_LOCAL : {
      DECODE_C();
      continue;
    }
    case CALL_OPCODE_CODE : {
      DECODE_C();
      continue;
    }
    case CALL_OPCODE_EXTERN : {
      DECODE_C();
      continue;
    }
    case CALL_CLOSURE_OPCODE : {
      DECODE_C();
      continue;
    }
    case TCALL_OPCODE_LOCAL : {
      DECODE_C();
      continue;
    }
    case TCALL_OPCODE_CODE : {
      DECODE_C();
      continue;
    }
    case TCALL_OPCODE_EXTERN : {
      DECODE_C();
      continue;
    }
    case TCALL_CLOSURE_OPCODE : {
      DECODE_A_UNSIGNED();
      continue;
    }
    case CALLC_OPCODE_LOCAL : {
      DECODE_C();
      continue;
    }
    case CALLC_OPCODE_CODE : {
      DECODE_C();
      continue;
    }
    case CALLC_OPCODE_EXTERN : {
      DECODE_C();
      continue;
    }
    case POP_FRAME_OPCODE : {
      DECODE_A_UNSIGNED();
      continue;
    }
    case LIVE_OPCODE : {
      DECODE_A_UNSIGNED();
      continue;
    }
    case YIELD_OPCODE : {
      DECODE_A_UNSIGNED();
      continue;
    }
    case RETURN_OPCODE : {
      DECODE_A_UNSIGNED();
      continue;
    }
    case DUMP_OPCODE : {
      DECODE_A_UNSIGNED();
      continue;
    }
    case INT_ADD_OPCODE : {break;}
    case INT_SUB_OPCODE : {break;}
    case INT_MUL_OPCODE : {break;}
    case INT_DIV_OPCODE : {break;}
    case INT_MOD_OPCODE : {break;}
    case INT_AND_OPCODE : {break;}
    case INT_OR_OPCODE : {break;}
    case INT_XOR_OPCODE : {break;}
    case INT_SHL_OPCODE : {break;}
    case INT_SHR_OPCODE : {break;}
    case INT_ASHR_OPCODE : {break;}
    case INT_LT_OPCODE : {break;}
    case INT_GT_OPCODE : {break;}
    case INT_LE_OPCODE : {break;}
    case INT_GE_OPCODE : {break;}
    case EQ_OPCODE_REF_REF : {break;}
    case EQ_OPCODE_REF : {break;}
    case EQ_OPCODE_BYTE : {break;}
    case EQ_OPCODE_INT : {break;}
    case EQ_OPCODE_LONG : {break;}
    case EQ_OPCODE_FLOAT : {break;}
    case EQ_OPCODE_DOUBLE : {break;}
    case NE_OPCODE_REF_REF : {break;}
    case NE_OPCODE_REF : {break;}
    case NE_OPCODE_BYTE : {break;}
    case NE_OPCODE_INT : {break;}
    case NE_OPCODE_LONG : {break;}
    case NE_OPCODE_FLOAT : {break;}
    case NE_OPCODE_DOUBLE : {break;}
    case ADD_OPCODE_BYTE : {break;}
    case ADD_OPCODE_INT : {break;}
    case ADD_OPCODE_LONG : {break;}
    case ADD_OPCODE_FLOAT : {break;}
    case ADD_OPCODE_DOUBLE : {break;}
    case SUB_OPCODE_BYTE : {break;}
    case SUB_OPCODE_INT : {break;}
    case SUB_OPCODE_LONG : {break;}
    case SUB_OPCODE_FLOAT : {break;}
    case SUB_OPCODE_DOUBLE : {break;}
    case MUL_OPCODE_BYTE : {break;}
    case MUL_OPCODE_INT : {break;}
    case MUL_OPCODE_LONG : {break;}
    case MUL_OPCODE_FLOAT : {break;}
    case MUL_OPCODE_DOUBLE : {break;}
    case DIV_OPCODE_BYTE : {break;}
    case DIV_OPCODE_INT : {break;}
    case DIV_OPCODE_LONG : {break;}
    case DIV_OPCODE_FLOAT : {break;}
    case DIV_OPCODE_DOUBLE : {break;}
    case MOD_OPCODE_BYTE : {break;}
    case MOD_OPCODE_INT : {break;}
    case MOD_OPCODE_LONG : {break;}
    case AND_OPCODE_BYTE : {break;}
    case AND_OPCODE_INT : {break;}
    case AND_OPCODE_LONG : {break;}
    case OR_OPCODE_BYTE : {break;}
    case OR_OPCODE_INT : {break;}
    case OR_OPCODE_LONG : {break;}
    case XOR_OPCODE_BYTE : {break;}
    case XOR_OPCODE_INT : {break;}
    case XOR_OPCODE_LONG : {break;}
    case SHL_OPCODE_BYTE : {break;}
    case SHL_OPCODE_INT : {break;}
    case SHL_OPCODE_LONG : {break;}
    case SHR_OPCODE_BYTE : {break;}
    case SHR_OPCODE_INT : {break;}
    case SHR_OPCODE_LONG : {break;}
    case ASHR_OPCODE_BYTE : {break;}
    case ASHR_OPCODE_INT : {break;}
    case ASHR_OPCODE_LONG : {break;}
    case LT_OPCODE_INT : {break;}
    case LT_OPCODE_LONG : {break;}
    case LT_OPCODE_FLOAT : {break;}
    case LT_OPCODE_DOUBLE : {break;}
    case GT_OPCODE_INT : {break;}
    case GT_OPCODE_LONG : {break;}
    case GT_OPCODE_FLOAT : {break;}
    case GT_OPCODE_DOUBLE : {break;}
    case LE_OPCODE_INT : {break;}
    case LE_OPCODE_LONG : {break;}
    case LE_OPCODE_FLOAT : {break;}
    case LE_OPCODE_DOUBLE : {break;}
    case GE_OPCODE_INT : {break;}
    case GE_OPCODE_LONG : {break;}
    case GE_OPCODE_FLOAT : {break;}
    case GE_OPCODE_DOUBLE : {break;}
    case ULE_OPCODE_BYTE : {break;}
    case ULE_OPCODE_INT : {break;}
    case ULE_OPCODE_LONG : {break;}
    case ULT_OPCODE_BYTE : {break;}
    case ULT_OPCODE_INT : {break;}
    case ULT_OPCODE_LONG : {break;}
    case UGT_OPCODE_BYTE : {break;}
    case UGT_OPCODE_INT : {break;}
    case UGT_OPCODE_LONG : {break;}
    case UGE_OPCODE_BYTE : {break;}
    case UGE_OPCODE_INT : {break;}
    case UGE_OPCODE_LONG : {break;}
    case INT_NOT_OPCODE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case INT_NEG_OPCODE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case NOT_OPCODE_BYTE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case NOT_OPCODE_INT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case NOT_OPCODE_LONG : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case NEG_OPCODE_INT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case NEG_OPCODE_LONG : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case NEG_OPCODE_FLOAT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case NEG_OPCODE_DOUBLE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case DEREF_OPCODE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case TYPEOF_OPCODE : {break;}
    case JUMP_SET_OPCODE : {break;}
    case GOTO_OPCODE : {
      DECODE_A_SIGNED();
      continue;
    }
    case CONV_OPCODE_BYTE_FLOAT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_BYTE_DOUBLE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_INT_BYTE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_INT_FLOAT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_INT_DOUBLE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_LONG_BYTE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_LONG_INT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_LONG_FLOAT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_LONG_DOUBLE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_FLOAT_BYTE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_FLOAT_INT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_FLOAT_LONG : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_FLOAT_DOUBLE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_DOUBLE_BYTE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_DOUBLE_INT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_DOUBLE_LONG : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CONV_OPCODE_DOUBLE_FLOAT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case DETAG_OPCODE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case TAG_OPCODE_BYTE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case TAG_OPCODE_CHAR : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case TAG_OPCODE_INT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case TAG_OPCODE_FLOAT : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case STORE_OPCODE : {
      DECODE_E();
      continue;
    }
    case STORE_OPCODE_VAR_OFFSET : {
      DECODE_E();
      continue;
    }
    case STORE_OPCODE_REF : {
      DECODE_E();
      continue;
    }
    case STORE_OPCODE_REF_VAR_OFFSET : {
      DECODE_E();
      continue;
    }
    case LOAD_OPCODE : {
      DECODE_E();
      continue;
    }
    case LOAD_OPCODE_VAR_OFFSET : {
      DECODE_E();
      continue;
    }
    case LOAD_OPCODE_REF : {
      DECODE_E();
      continue;
    }
    case LOAD_OPCODE_REF_VAR_OFFSET : {
      DECODE_E();
      continue;
    }
    case RESERVE_OPCODE_LOCAL : {break;}
    case RESERVE_OPCODE_CONST : {break;}
    case NEW_STACK_OPCODE : {break;}
    case ALLOC_OPCODE_CONST : {break;}
    case ALLOC_OPCODE_LOCAL : {break;}
    case GC_OPCODE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case PRINT_STACK_TRACE_OPCODE : {
      DECODE_B_UNSIGNED();
      continue;
    }
    case CURRENT_STACK_OPCODE : {
      DECODE_A_UNSIGNED();
      continue;
    }
    case FLUSH_VM_OPCODE : {
      DECODE_A_UNSIGNED();
      continue;
    }
    case GLOBALS_OPCODE : {
      DECODE_A_UNSIGNED();
      continue;
    }
    case CONSTS_OPCODE : {
      DECODE_A_UNSIGNED();
      continue;
    }
    case CONSTS_DATA_OPCODE : {
      DECODE_A_UNSIGNED();
      continue;
    }
    case JUMP_INT_LT_OPCODE : {
      DECODE_F();
      continue;
    }
    case JUMP_INT_GT_OPCODE : {
      DECODE_F();
      continue;
    }
    case JUMP_INT_LE_OPCODE : {
      DECODE_F();
      continue;
    }
    case JUMP_INT_GE_OPCODE : {
      DECODE_F();
      continue;
    }
    case JUMP_EQ_OPCODE_REF : {
      DECODE_F();
      continue;
    }
    case JUMP_EQ_OPCODE_BYTE : {
      DECODE_F();
      continue;
    }
    case JUMP_EQ_OPCODE_INT : {
      DECODE_F();
      continue;
    }
    case JUMP_EQ_OPCODE_LONG : {
      DECODE_F();
      continue;
    }
    case JUMP_EQ_OPCODE_FLOAT : {
      DECODE_F();
      continue;
    }
    case JUMP_EQ_OPCODE_DOUBLE : {
      DECODE_F();
      continue;
    }
    case JUMP_NE_OPCODE_REF : {
      DECODE_F();
      continue;
    }
    case JUMP_NE_OPCODE_BYTE : {
      DECODE_F();
      continue;
    }
    case JUMP_NE_OPCODE_INT : {
      DECODE_F();
      continue;
    }
    case JUMP_NE_OPCODE_LONG : {
      DECODE_F();
      continue;
    }
    case JUMP_NE_OPCODE_FLOAT : {
      DECODE_F();
      continue;
    }
    case JUMP_NE_OPCODE_DOUBLE : {
      DECODE_F();
      continue;
    }
    case JUMP_LT_OPCODE_INT : {
      DECODE_F();
      continue;
    }
    case JUMP_LT_OPCODE_LONG : {
      DECODE_F();
      continue;
    }
    case JUMP_LT_OPCODE_FLOAT : {
      DECODE_F();
      continue;
    }
    case JUMP_LT_OPCODE_DOUBLE : {
      DECODE_F();
      continue;
    }
    case JUMP_GT_OPCODE_INT : {
      DECODE_F();
      continue;
    }
    case JUMP_GT_OPCODE_LONG : {
      DECODE_F();
      continue;
    }
    case JUMP_GT_OPCODE_FLOAT : {
      DECODE_F();
      continue;
    }
    case JUMP_GT_OPCODE_DOUBLE : {
      DECODE_F();
      continue;
    }
    case JUMP_LE_OPCODE_INT : {
      DECODE_F();
      continue;
    }
    case JUMP_LE_OPCODE_LONG : {
      DECODE_F();
      continue;
    }
    case JUMP_LE_OPCODE_FLOAT : {
      DECODE_F();
      continue;
    }
    case JUMP_LE_OPCODE_DOUBLE : {
      DECODE_F();
      continue;
    }
    case JUMP_GE_OPCODE_INT : {
      DECODE_F();
      continue;
    }
    case JUMP_GE_OPCODE_LONG : {
      DECODE_F();
      continue;
    }
    case JUMP_GE_OPCODE_FLOAT : {
      DECODE_F();
      continue;
    }
    case JUMP_GE_OPCODE_DOUBLE : {
      DECODE_F();
      continue;
    }
    case JUMP_ULE_OPCODE_BYTE : {
      DECODE_F();
      continue;
    }
    case JUMP_ULE_OPCODE_INT : {
      DECODE_F();
      continue;
    }
    case JUMP_ULE_OPCODE_LONG : {
      DECODE_F();
      continue;
    }
    case JUMP_ULT_OPCODE_BYTE : {
      DECODE_F();
      continue;
    }
    case JUMP_ULT_OPCODE_INT : {
      DECODE_F();
      continue;
    }
    case JUMP_ULT_OPCODE_LONG : {
      DECODE_F();
      continue;
    }
    case JUMP_UGT_OPCODE_BYTE : {
      DECODE_F();
      continue;
    }
    case JUMP_UGT_OPCODE_INT : {
      DECODE_F();
      continue;
    }
    case JUMP_UGT_OPCODE_LONG : {
      DECODE_F();
      continue;
    }
    case JUMP_UGE_OPCODE_BYTE : {
      DECODE_F();
      continue;
    }
    case JUMP_UGE_OPCODE_INT : {
      DECODE_F();
      continue;
    }
    case JUMP_UGE_OPCODE_LONG : {
      DECODE_F();
      continue;
    }
    case DISPATCH_OPCODE : {
      DECODE_A_UNSIGNED();
      DECODE_TGTS();
      continue;
    }
    case DISPATCH_METHOD_OPCODE : {
      DECODE_A_UNSIGNED();
      DECODE_TGTS();
      continue;
    }
    case JUMP_REG_OPCODE : {break;}
    }

    //Done
    printf("opcode = %d\n", opcode);
    printf("W1 = %x\n", W1);
    exit(-1);
    return;
  }
}
