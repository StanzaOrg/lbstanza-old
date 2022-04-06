#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<stdint.h>
#include<inttypes.h>

//============================================================
//=================== OPCODES ================================
//============================================================

#define SET_OPCODE_LOCAL 0
#define SET_OPCODE_UNSIGNED 1
#define SET_OPCODE_SIGNED 2
#define SET_OPCODE_CODE 3
#define SET_OPCODE_GLOBAL 5
#define SET_OPCODE_DATA 6
#define SET_OPCODE_CONST 7
#define SET_OPCODE_WIDE 8
#define SET_REG_OPCODE_LOCAL 9
#define SET_REG_OPCODE_UNSIGNED 10
#define SET_REG_OPCODE_SIGNED 11
#define SET_REG_OPCODE_CODE 12
#define SET_REG_OPCODE_GLOBAL 14
#define SET_REG_OPCODE_DATA 15
#define SET_REG_OPCODE_CONST 16
#define SET_REG_OPCODE_WIDE 17
#define GET_REG_OPCODE 18
#define CALL_OPCODE_LOCAL 19
#define CALL_OPCODE_CODE 20
#define CALL_CLOSURE_OPCODE 22
#define TCALL_OPCODE_LOCAL 23
#define TCALL_OPCODE_CODE 24
#define TCALL_CLOSURE_OPCODE 26
#define CALLC_OPCODE_LOCAL 27
#define CALLC_OPCODE_WIDE 28
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
#define REF_EQ_OPCODE 50
#define EQ_OPCODE_REF 51
#define EQ_OPCODE_BYTE 52
#define EQ_OPCODE_INT 53
#define EQ_OPCODE_LONG 54
#define EQ_OPCODE_FLOAT 55
#define EQ_OPCODE_DOUBLE 56
#define REF_NE_OPCODE 57
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
#define JUMP_TAGBITS_OPCODE 240
#define JUMP_TAGWORD_OPCODE 242
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
#define ENTER_STACK_OPCODE 182
#define ALLOC_OPCODE_CONST 183
#define ALLOC_OPCODE_LOCAL 184
#define GC_OPCODE 185
#define PRINT_STACK_TRACE_OPCODE 186
#define COLLECT_STACK_TRACE_OPCODE 187
#define FLUSH_VM_OPCODE 188
#define C_RSP_OPCODE 243
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
#define FNENTRY_OPCODE 239
#define LOWEST_ZERO_BIT_COUNT_OPCODE_LONG 244
#define TEST_BIT_OPCODE 245
#define SET_BIT_OPCODE 246
#define CLEAR_BIT_OPCODE 247
#define TEST_AND_SET_BIT_OPCODE 248
#define TEST_AND_CLEAR_BIT_OPCODE 249
#define STORE_WITH_BARRIER_OPCODE 250
#define STORE_WITH_BARRIER_OPCODE_VAR_OFFSET 251

char* opcode_names[256];
void init_opcode_names () {
  opcode_names[SET_OPCODE_LOCAL] = "SET_OPCODE_LOCAL";
  opcode_names[SET_OPCODE_UNSIGNED] = "SET_OPCODE_UNSIGNED";
  opcode_names[SET_OPCODE_SIGNED] = "SET_OPCODE_SIGNED";
  opcode_names[SET_OPCODE_CODE] = "SET_OPCODE_CODE";
  opcode_names[SET_OPCODE_GLOBAL] = "SET_OPCODE_GLOBAL";
  opcode_names[SET_OPCODE_DATA] = "SET_OPCODE_DATA";
  opcode_names[SET_OPCODE_CONST] = "SET_OPCODE_CONST";
  opcode_names[SET_OPCODE_WIDE] = "SET_OPCODE_WIDE";
  opcode_names[SET_REG_OPCODE_LOCAL] = "SET_REG_OPCODE_LOCAL";
  opcode_names[SET_REG_OPCODE_UNSIGNED] = "SET_REG_OPCODE_UNSIGNED";
  opcode_names[SET_REG_OPCODE_SIGNED] = "SET_REG_OPCODE_SIGNED";
  opcode_names[SET_REG_OPCODE_CODE] = "SET_REG_OPCODE_CODE";
  opcode_names[SET_REG_OPCODE_GLOBAL] = "SET_REG_OPCODE_GLOBAL";
  opcode_names[SET_REG_OPCODE_DATA] = "SET_REG_OPCODE_DATA";
  opcode_names[SET_REG_OPCODE_CONST] = "SET_REG_OPCODE_CONST";
  opcode_names[SET_REG_OPCODE_WIDE] = "SET_REG_OPCODE_WIDE";
  opcode_names[GET_REG_OPCODE] = "GET_REG_OPCODE";
  opcode_names[CALL_OPCODE_LOCAL] = "CALL_OPCODE_LOCAL";
  opcode_names[CALL_OPCODE_CODE] = "CALL_OPCODE_CODE";
  opcode_names[CALL_CLOSURE_OPCODE] = "CALL_CLOSURE_OPCODE";
  opcode_names[TCALL_OPCODE_LOCAL] = "TCALL_OPCODE_LOCAL";
  opcode_names[TCALL_OPCODE_CODE] = "TCALL_OPCODE_CODE";
  opcode_names[TCALL_CLOSURE_OPCODE] = "TCALL_CLOSURE_OPCODE";
  opcode_names[CALLC_OPCODE_LOCAL] = "CALLC_OPCODE_LOCAL";
  opcode_names[CALLC_OPCODE_WIDE] = "CALLC_OPCODE_WIDE";
  opcode_names[POP_FRAME_OPCODE] = "POP_FRAME_OPCODE";
  opcode_names[LIVE_OPCODE] = "LIVE_OPCODE";
  opcode_names[YIELD_OPCODE] = "YIELD_OPCODE";
  opcode_names[RETURN_OPCODE] = "RETURN_OPCODE";
  opcode_names[DUMP_OPCODE] = "DUMP_OPCODE";
  opcode_names[INT_ADD_OPCODE] = "INT_ADD_OPCODE";
  opcode_names[INT_SUB_OPCODE] = "INT_SUB_OPCODE";
  opcode_names[INT_MUL_OPCODE] = "INT_MUL_OPCODE";
  opcode_names[INT_DIV_OPCODE] = "INT_DIV_OPCODE";
  opcode_names[INT_MOD_OPCODE] = "INT_MOD_OPCODE";
  opcode_names[INT_AND_OPCODE] = "INT_AND_OPCODE";
  opcode_names[INT_OR_OPCODE] = "INT_OR_OPCODE";
  opcode_names[INT_XOR_OPCODE] = "INT_XOR_OPCODE";
  opcode_names[INT_SHL_OPCODE] = "INT_SHL_OPCODE";
  opcode_names[INT_SHR_OPCODE] = "INT_SHR_OPCODE";
  opcode_names[INT_ASHR_OPCODE] = "INT_ASHR_OPCODE";
  opcode_names[INT_LT_OPCODE] = "INT_LT_OPCODE";
  opcode_names[INT_GT_OPCODE] = "INT_GT_OPCODE";
  opcode_names[INT_LE_OPCODE] = "INT_LE_OPCODE";
  opcode_names[INT_GE_OPCODE] = "INT_GE_OPCODE";
  opcode_names[REF_EQ_OPCODE] = "REF_EQ_OPCODE";
  opcode_names[EQ_OPCODE_REF] = "EQ_OPCODE_REF";
  opcode_names[EQ_OPCODE_BYTE] = "EQ_OPCODE_BYTE";
  opcode_names[EQ_OPCODE_INT] = "EQ_OPCODE_INT";
  opcode_names[EQ_OPCODE_LONG] = "EQ_OPCODE_LONG";
  opcode_names[EQ_OPCODE_FLOAT] = "EQ_OPCODE_FLOAT";
  opcode_names[EQ_OPCODE_DOUBLE] = "EQ_OPCODE_DOUBLE";
  opcode_names[REF_NE_OPCODE] = "REF_NE_OPCODE";
  opcode_names[NE_OPCODE_REF] = "NE_OPCODE_REF";
  opcode_names[NE_OPCODE_BYTE] = "NE_OPCODE_BYTE";
  opcode_names[NE_OPCODE_INT] = "NE_OPCODE_INT";
  opcode_names[NE_OPCODE_LONG] = "NE_OPCODE_LONG";
  opcode_names[NE_OPCODE_FLOAT] = "NE_OPCODE_FLOAT";
  opcode_names[NE_OPCODE_DOUBLE] = "NE_OPCODE_DOUBLE";
  opcode_names[ADD_OPCODE_BYTE] = "ADD_OPCODE_BYTE";
  opcode_names[ADD_OPCODE_INT] = "ADD_OPCODE_INT";
  opcode_names[ADD_OPCODE_LONG] = "ADD_OPCODE_LONG";
  opcode_names[ADD_OPCODE_FLOAT] = "ADD_OPCODE_FLOAT";
  opcode_names[ADD_OPCODE_DOUBLE] = "ADD_OPCODE_DOUBLE";
  opcode_names[SUB_OPCODE_BYTE] = "SUB_OPCODE_BYTE";
  opcode_names[SUB_OPCODE_INT] = "SUB_OPCODE_INT";
  opcode_names[SUB_OPCODE_LONG] = "SUB_OPCODE_LONG";
  opcode_names[SUB_OPCODE_FLOAT] = "SUB_OPCODE_FLOAT";
  opcode_names[SUB_OPCODE_DOUBLE] = "SUB_OPCODE_DOUBLE";
  opcode_names[MUL_OPCODE_BYTE] = "MUL_OPCODE_BYTE";
  opcode_names[MUL_OPCODE_INT] = "MUL_OPCODE_INT";
  opcode_names[MUL_OPCODE_LONG] = "MUL_OPCODE_LONG";
  opcode_names[MUL_OPCODE_FLOAT] = "MUL_OPCODE_FLOAT";
  opcode_names[MUL_OPCODE_DOUBLE] = "MUL_OPCODE_DOUBLE";
  opcode_names[DIV_OPCODE_BYTE] = "DIV_OPCODE_BYTE";
  opcode_names[DIV_OPCODE_INT] = "DIV_OPCODE_INT";
  opcode_names[DIV_OPCODE_LONG] = "DIV_OPCODE_LONG";
  opcode_names[DIV_OPCODE_FLOAT] = "DIV_OPCODE_FLOAT";
  opcode_names[DIV_OPCODE_DOUBLE] = "DIV_OPCODE_DOUBLE";
  opcode_names[MOD_OPCODE_BYTE] = "MOD_OPCODE_BYTE";
  opcode_names[MOD_OPCODE_INT] = "MOD_OPCODE_INT";
  opcode_names[MOD_OPCODE_LONG] = "MOD_OPCODE_LONG";
  opcode_names[AND_OPCODE_BYTE] = "AND_OPCODE_BYTE";
  opcode_names[AND_OPCODE_INT] = "AND_OPCODE_INT";
  opcode_names[AND_OPCODE_LONG] = "AND_OPCODE_LONG";
  opcode_names[OR_OPCODE_BYTE] = "OR_OPCODE_BYTE";
  opcode_names[OR_OPCODE_INT] = "OR_OPCODE_INT";
  opcode_names[OR_OPCODE_LONG] = "OR_OPCODE_LONG";
  opcode_names[XOR_OPCODE_BYTE] = "XOR_OPCODE_BYTE";
  opcode_names[XOR_OPCODE_INT] = "XOR_OPCODE_INT";
  opcode_names[XOR_OPCODE_LONG] = "XOR_OPCODE_LONG";
  opcode_names[SHL_OPCODE_BYTE] = "SHL_OPCODE_BYTE";
  opcode_names[SHL_OPCODE_INT] = "SHL_OPCODE_INT";
  opcode_names[SHL_OPCODE_LONG] = "SHL_OPCODE_LONG";
  opcode_names[SHR_OPCODE_BYTE] = "SHR_OPCODE_BYTE";
  opcode_names[SHR_OPCODE_INT] = "SHR_OPCODE_INT";
  opcode_names[SHR_OPCODE_LONG] = "SHR_OPCODE_LONG";
  opcode_names[ASHR_OPCODE_INT] = "ASHR_OPCODE_INT";
  opcode_names[ASHR_OPCODE_LONG] = "ASHR_OPCODE_LONG";
  opcode_names[LT_OPCODE_INT] = "LT_OPCODE_INT";
  opcode_names[LT_OPCODE_LONG] = "LT_OPCODE_LONG";
  opcode_names[LT_OPCODE_FLOAT] = "LT_OPCODE_FLOAT";
  opcode_names[LT_OPCODE_DOUBLE] = "LT_OPCODE_DOUBLE";
  opcode_names[GT_OPCODE_INT] = "GT_OPCODE_INT";
  opcode_names[GT_OPCODE_LONG] = "GT_OPCODE_LONG";
  opcode_names[GT_OPCODE_FLOAT] = "GT_OPCODE_FLOAT";
  opcode_names[GT_OPCODE_DOUBLE] = "GT_OPCODE_DOUBLE";
  opcode_names[LE_OPCODE_INT] = "LE_OPCODE_INT";
  opcode_names[LE_OPCODE_LONG] = "LE_OPCODE_LONG";
  opcode_names[LE_OPCODE_FLOAT] = "LE_OPCODE_FLOAT";
  opcode_names[LE_OPCODE_DOUBLE] = "LE_OPCODE_DOUBLE";
  opcode_names[GE_OPCODE_INT] = "GE_OPCODE_INT";
  opcode_names[GE_OPCODE_LONG] = "GE_OPCODE_LONG";
  opcode_names[GE_OPCODE_FLOAT] = "GE_OPCODE_FLOAT";
  opcode_names[GE_OPCODE_DOUBLE] = "GE_OPCODE_DOUBLE";
  opcode_names[ULE_OPCODE_BYTE] = "ULE_OPCODE_BYTE";
  opcode_names[ULE_OPCODE_INT] = "ULE_OPCODE_INT";
  opcode_names[ULE_OPCODE_LONG] = "ULE_OPCODE_LONG";
  opcode_names[ULT_OPCODE_BYTE] = "ULT_OPCODE_BYTE";
  opcode_names[ULT_OPCODE_INT] = "ULT_OPCODE_INT";
  opcode_names[ULT_OPCODE_LONG] = "ULT_OPCODE_LONG";
  opcode_names[UGT_OPCODE_BYTE] = "UGT_OPCODE_BYTE";
  opcode_names[UGT_OPCODE_INT] = "UGT_OPCODE_INT";
  opcode_names[UGT_OPCODE_LONG] = "UGT_OPCODE_LONG";
  opcode_names[UGE_OPCODE_BYTE] = "UGE_OPCODE_BYTE";
  opcode_names[UGE_OPCODE_INT] = "UGE_OPCODE_INT";
  opcode_names[UGE_OPCODE_LONG] = "UGE_OPCODE_LONG";
  opcode_names[INT_NOT_OPCODE] = "INT_NOT_OPCODE";
  opcode_names[INT_NEG_OPCODE] = "INT_NEG_OPCODE";
  opcode_names[NOT_OPCODE_BYTE] = "NOT_OPCODE_BYTE";
  opcode_names[NOT_OPCODE_INT] = "NOT_OPCODE_INT";
  opcode_names[NOT_OPCODE_LONG] = "NOT_OPCODE_LONG";
  opcode_names[NEG_OPCODE_INT] = "NEG_OPCODE_INT";
  opcode_names[NEG_OPCODE_LONG] = "NEG_OPCODE_LONG";
  opcode_names[NEG_OPCODE_FLOAT] = "NEG_OPCODE_FLOAT";
  opcode_names[NEG_OPCODE_DOUBLE] = "NEG_OPCODE_DOUBLE";
  opcode_names[DEREF_OPCODE] = "DEREF_OPCODE";
  opcode_names[TYPEOF_OPCODE] = "TYPEOF_OPCODE";
  opcode_names[JUMP_SET_OPCODE] = "JUMP_SET_OPCODE";
  opcode_names[JUMP_TAGBITS_OPCODE] = "JUMP_TAGBITS_OPCODE";
  opcode_names[JUMP_TAGWORD_OPCODE] = "JUMP_TAGWORD_OPCODE";
  opcode_names[GOTO_OPCODE] = "GOTO_OPCODE";
  opcode_names[CONV_OPCODE_BYTE_FLOAT] = "CONV_OPCODE_BYTE_FLOAT";
  opcode_names[CONV_OPCODE_BYTE_DOUBLE] = "CONV_OPCODE_BYTE_DOUBLE";
  opcode_names[CONV_OPCODE_INT_BYTE] = "CONV_OPCODE_INT_BYTE";
  opcode_names[CONV_OPCODE_INT_FLOAT] = "CONV_OPCODE_INT_FLOAT";
  opcode_names[CONV_OPCODE_INT_DOUBLE] = "CONV_OPCODE_INT_DOUBLE";
  opcode_names[CONV_OPCODE_LONG_BYTE] = "CONV_OPCODE_LONG_BYTE";
  opcode_names[CONV_OPCODE_LONG_INT] = "CONV_OPCODE_LONG_INT";
  opcode_names[CONV_OPCODE_LONG_FLOAT] = "CONV_OPCODE_LONG_FLOAT";
  opcode_names[CONV_OPCODE_LONG_DOUBLE] = "CONV_OPCODE_LONG_DOUBLE";
  opcode_names[CONV_OPCODE_FLOAT_BYTE] = "CONV_OPCODE_FLOAT_BYTE";
  opcode_names[CONV_OPCODE_FLOAT_INT] = "CONV_OPCODE_FLOAT_INT";
  opcode_names[CONV_OPCODE_FLOAT_LONG] = "CONV_OPCODE_FLOAT_LONG";
  opcode_names[CONV_OPCODE_FLOAT_DOUBLE] = "CONV_OPCODE_FLOAT_DOUBLE";
  opcode_names[CONV_OPCODE_DOUBLE_BYTE] = "CONV_OPCODE_DOUBLE_BYTE";
  opcode_names[CONV_OPCODE_DOUBLE_INT] = "CONV_OPCODE_DOUBLE_INT";
  opcode_names[CONV_OPCODE_DOUBLE_LONG] = "CONV_OPCODE_DOUBLE_LONG";
  opcode_names[CONV_OPCODE_DOUBLE_FLOAT] = "CONV_OPCODE_DOUBLE_FLOAT";
  opcode_names[DETAG_OPCODE] = "DETAG_OPCODE";
  opcode_names[TAG_OPCODE_BYTE] = "TAG_OPCODE_BYTE";
  opcode_names[TAG_OPCODE_CHAR] = "TAG_OPCODE_CHAR";
  opcode_names[TAG_OPCODE_INT] = "TAG_OPCODE_INT";
  opcode_names[TAG_OPCODE_FLOAT] = "TAG_OPCODE_FLOAT";
  opcode_names[STORE_OPCODE_1] = "STORE_OPCODE_1";
  opcode_names[STORE_OPCODE_4] = "STORE_OPCODE_4";
  opcode_names[STORE_OPCODE_8] = "STORE_OPCODE_8";
  opcode_names[STORE_OPCODE_1_VAR_OFFSET] = "STORE_OPCODE_1_VAR_OFFSET";
  opcode_names[STORE_OPCODE_4_VAR_OFFSET] = "STORE_OPCODE_4_VAR_OFFSET";
  opcode_names[STORE_OPCODE_8_VAR_OFFSET] = "STORE_OPCODE_8_VAR_OFFSET";
  opcode_names[LOAD_OPCODE_1] = "LOAD_OPCODE_1";
  opcode_names[LOAD_OPCODE_4] = "LOAD_OPCODE_4";
  opcode_names[LOAD_OPCODE_8] = "LOAD_OPCODE_8";
  opcode_names[LOAD_OPCODE_1_VAR_OFFSET] = "LOAD_OPCODE_1_VAR_OFFSET";
  opcode_names[LOAD_OPCODE_4_VAR_OFFSET] = "LOAD_OPCODE_4_VAR_OFFSET";
  opcode_names[LOAD_OPCODE_8_VAR_OFFSET] = "LOAD_OPCODE_8_VAR_OFFSET";
  opcode_names[RESERVE_OPCODE_LOCAL] = "RESERVE_OPCODE_LOCAL";
  opcode_names[RESERVE_OPCODE_CONST] = "RESERVE_OPCODE_CONST";
  opcode_names[ENTER_STACK_OPCODE] = "ENTER_STACK_OPCODE";
  opcode_names[ALLOC_OPCODE_CONST] = "ALLOC_OPCODE_CONST";
  opcode_names[ALLOC_OPCODE_LOCAL] = "ALLOC_OPCODE_LOCAL";
  opcode_names[GC_OPCODE] = "GC_OPCODE";
  opcode_names[PRINT_STACK_TRACE_OPCODE] = "PRINT_STACK_TRACE_OPCODE";
  opcode_names[COLLECT_STACK_TRACE_OPCODE] = "COLLECT_STACK_TRACE_OPCODE";
  opcode_names[FLUSH_VM_OPCODE] = "FLUSH_VM_OPCODE";
  opcode_names[C_RSP_OPCODE] = "C_RSP_OPCODE";
  opcode_names[JUMP_INT_LT_OPCODE] = "JUMP_INT_LT_OPCODE";
  opcode_names[JUMP_INT_GT_OPCODE] = "JUMP_INT_GT_OPCODE";
  opcode_names[JUMP_INT_LE_OPCODE] = "JUMP_INT_LE_OPCODE";
  opcode_names[JUMP_INT_GE_OPCODE] = "JUMP_INT_GE_OPCODE";
  opcode_names[JUMP_EQ_OPCODE_REF] = "JUMP_EQ_OPCODE_REF";
  opcode_names[JUMP_EQ_OPCODE_BYTE] = "JUMP_EQ_OPCODE_BYTE";
  opcode_names[JUMP_EQ_OPCODE_INT] = "JUMP_EQ_OPCODE_INT";
  opcode_names[JUMP_EQ_OPCODE_LONG] = "JUMP_EQ_OPCODE_LONG";
  opcode_names[JUMP_EQ_OPCODE_FLOAT] = "JUMP_EQ_OPCODE_FLOAT";
  opcode_names[JUMP_EQ_OPCODE_DOUBLE] = "JUMP_EQ_OPCODE_DOUBLE";
  opcode_names[JUMP_NE_OPCODE_REF] = "JUMP_NE_OPCODE_REF";
  opcode_names[JUMP_NE_OPCODE_BYTE] = "JUMP_NE_OPCODE_BYTE";
  opcode_names[JUMP_NE_OPCODE_INT] = "JUMP_NE_OPCODE_INT";
  opcode_names[JUMP_NE_OPCODE_LONG] = "JUMP_NE_OPCODE_LONG";
  opcode_names[JUMP_NE_OPCODE_FLOAT] = "JUMP_NE_OPCODE_FLOAT";
  opcode_names[JUMP_NE_OPCODE_DOUBLE] = "JUMP_NE_OPCODE_DOUBLE";
  opcode_names[JUMP_LT_OPCODE_INT] = "JUMP_LT_OPCODE_INT";
  opcode_names[JUMP_LT_OPCODE_LONG] = "JUMP_LT_OPCODE_LONG";
  opcode_names[JUMP_LT_OPCODE_FLOAT] = "JUMP_LT_OPCODE_FLOAT";
  opcode_names[JUMP_LT_OPCODE_DOUBLE] = "JUMP_LT_OPCODE_DOUBLE";
  opcode_names[JUMP_GT_OPCODE_INT] = "JUMP_GT_OPCODE_INT";
  opcode_names[JUMP_GT_OPCODE_LONG] = "JUMP_GT_OPCODE_LONG";
  opcode_names[JUMP_GT_OPCODE_FLOAT] = "JUMP_GT_OPCODE_FLOAT";
  opcode_names[JUMP_GT_OPCODE_DOUBLE] = "JUMP_GT_OPCODE_DOUBLE";
  opcode_names[JUMP_LE_OPCODE_INT] = "JUMP_LE_OPCODE_INT";
  opcode_names[JUMP_LE_OPCODE_LONG] = "JUMP_LE_OPCODE_LONG";
  opcode_names[JUMP_LE_OPCODE_FLOAT] = "JUMP_LE_OPCODE_FLOAT";
  opcode_names[JUMP_LE_OPCODE_DOUBLE] = "JUMP_LE_OPCODE_DOUBLE";
  opcode_names[JUMP_GE_OPCODE_INT] = "JUMP_GE_OPCODE_INT";
  opcode_names[JUMP_GE_OPCODE_LONG] = "JUMP_GE_OPCODE_LONG";
  opcode_names[JUMP_GE_OPCODE_FLOAT] = "JUMP_GE_OPCODE_FLOAT";
  opcode_names[JUMP_GE_OPCODE_DOUBLE] = "JUMP_GE_OPCODE_DOUBLE";
  opcode_names[JUMP_ULE_OPCODE_BYTE] = "JUMP_ULE_OPCODE_BYTE";
  opcode_names[JUMP_ULE_OPCODE_INT] = "JUMP_ULE_OPCODE_INT";
  opcode_names[JUMP_ULE_OPCODE_LONG] = "JUMP_ULE_OPCODE_LONG";
  opcode_names[JUMP_ULT_OPCODE_BYTE] = "JUMP_ULT_OPCODE_BYTE";
  opcode_names[JUMP_ULT_OPCODE_INT] = "JUMP_ULT_OPCODE_INT";
  opcode_names[JUMP_ULT_OPCODE_LONG] = "JUMP_ULT_OPCODE_LONG";
  opcode_names[JUMP_UGT_OPCODE_BYTE] = "JUMP_UGT_OPCODE_BYTE";
  opcode_names[JUMP_UGT_OPCODE_INT] = "JUMP_UGT_OPCODE_INT";
  opcode_names[JUMP_UGT_OPCODE_LONG] = "JUMP_UGT_OPCODE_LONG";
  opcode_names[JUMP_UGE_OPCODE_BYTE] = "JUMP_UGE_OPCODE_BYTE";
  opcode_names[JUMP_UGE_OPCODE_INT] = "JUMP_UGE_OPCODE_INT";
  opcode_names[JUMP_UGE_OPCODE_LONG] = "JUMP_UGE_OPCODE_LONG";
  opcode_names[DISPATCH_OPCODE] = "DISPATCH_OPCODE";
  opcode_names[DISPATCH_METHOD_OPCODE] = "DISPATCH_METHOD_OPCODE";
  opcode_names[JUMP_REG_OPCODE] = "JUMP_REG_OPCODE";
  opcode_names[FNENTRY_OPCODE] = "FNENTRY_OPCODE";
  opcode_names[LOWEST_ZERO_BIT_COUNT_OPCODE_LONG] = "LOWEST_ZERO_BIT_COUNT_OPCODE_LONG";
  opcode_names[TEST_BIT_OPCODE] = "TEST_BIT_OPCODE";
  opcode_names[SET_BIT_OPCODE] = "SET_BIT_OPCODE";
  opcode_names[CLEAR_BIT_OPCODE] = "CLEAR_BIT_OPCODE";
  opcode_names[TEST_AND_SET_BIT_OPCODE] = "TEST_AND_SET_BIT_OPCODE";
  opcode_names[TEST_AND_CLEAR_BIT_OPCODE] = "TEST_AND_CLEAR_BIT_OPCODE";
  opcode_names[STORE_WITH_BARRIER_OPCODE] = "STORE_WITH_BARRIER_OPCODE";
  opcode_names[STORE_WITH_BARRIER_OPCODE_VAR_OFFSET] = "STORE_WITH_BARRIER_OPCODE_VAR_OFFSET";
}

//============================================================
//===================== READ MACROS ==========================
//============================================================

#define PC_INT() \
  ({uint32_t _x = *(uint32_t*)pc; \
    pc += 4; \
    _x;});

#define PC_LONG() \
  ({uint64_t _x = *(uint64_t*)pc; \
    pc += 8; \
    _x;});

#define DECODE_A_UNSIGNED() \
  int32_t value = W1 >> 8; \
  /*if(iprint) printf("          %ld) [%d | %d]\n", icounter, opcode, value);*/

#define DECODE_A_SIGNED() \
  int32_t value = (int32_t)W1 >> 8; \
  /*if(iprint) printf("          %ld) [%d | %d]\n", icounter, opcode, value);*/

#define DECODE_B_UNSIGNED() \
  int32_t x = (W1 >> 8) & 0x3FF; \
  int32_t value = W1 >> 18; \
  /*if(iprint) printf("          %ld) [%d | %d | %d]\n", icounter, opcode, x, value);*/

#define DECODE_C() \
  int32_t x = (W1 >> 8) & 0x3FF; \
  int32_t y = (W1 >> 22) & 0x3FF; \
  uint32_t value = PC_INT(); \
  /*if(iprint) printf("          %ld) [%d | %d | %d | %d]\n", icounter, opcode, x, y, value);*/

#define DECODE_D() \
  uint32_t x = (W1 >> 22) & 0x3FF; \
  uint64_t value = PC_LONG(); \
  /*if(iprint) printf("          %ld) [%d | _ | %d | %ld]\n", icounter, opcode, x, value);*/

#define DECODE_E() \
  uint32_t W2 = PC_INT(); \
  uint64_t W12 = W1 | ((uint64_t)W2 << 32); \
  int32_t x = (int32_t)(W12 >> 8) & 0x3FF;  \
  int32_t y = (int32_t)(W12 >> 18) & 0x3FF; \
  int32_t z = (int32_t)(W12 >> 28) & 0x3FF; \
  int32_t value = (int32_t)((int64_t)W12 >> 38); \
  /*if(iprint) printf("          %ld) [%d | %d | %d | %d | %d]\n", icounter, opcode, x, y, z, value);*/

#define DECODE_F() \
  uint32_t W2 = PC_INT(); \
  uint64_t W12 = W1 | ((uint64_t)W2 << 32); \
  int32_t x = (int32_t)(W12 >> 8) & 0x3FF;  \
  int32_t y = (int32_t)(W12 >> 18) & 0x3FF; \
  int32_t _n1 = (int32_t)(W12 >> 14); /*Move first bit to 32-bit boundary*/ \
  int32_t n1 = (int32_t)(_n1 >> 14); /*Extend sign-bit*/ \
  int32_t n2 = (int32_t)((int32_t)W2 >> 14); /*Extend sign-bit of first word*/ \
  /*if(iprint) printf("          %ld) [%d | %d | %d | %d | %d]\n", icounter, opcode, x, y, n1, n2);*/

#define F_JUMP(condition) \
  if(condition){ \
    pc = pc0 + (n1 * 4); \
    continue; \
  } \
  else{ \
    pc = pc0 + (n2 * 4); \
    continue; \
  }

#define DECODE_TGTS() \
  uint32_t n = PC_INT(); \
  for(int i=0; i<n; i++){ \
    uint32_t tgt = PC_INT(); \
    /*printf("            tgt: %d\n", tgt);*/ \
  }

#define SET_REG(r,v) \
  registers[r] = v
#define SET_LOCAL(l,v) \
  stack_pointer->slots[l] = v;
#define SET_LOCAL_FLOAT(l,v) \
  float* addr = (float*)&(stack_pointer->slots[l]); \
  *addr = v;
#define SET_LOCAL_DOUBLE(l,v) \
  double* addr = (double*)&(stack_pointer->slots[l]); \
  *addr = v;

#define LOCAL(l) (stack_pointer->slots[l])
#define LOCAL_FLOAT(l) \
  ({float* addr = (float*)&(stack_pointer->slots[l]); \
    *addr;})
#define LOCAL_DOUBLE(l) \
  ({double* addr = (double*)&(stack_pointer->slots[l]); \
    *addr;})

#define PUSH_FRAME(num_locals) \
  stack_pointer = (StackFrame*)((char*)stack_pointer + sizeof(StackFrame) + (num_locals) * 8); \
  stack_pointer->returnpc = (uint64_t)(pc - instructions);

#define POP_FRAME(num_locals) \
  stack_pointer = (StackFrame*)((char*)stack_pointer - sizeof(StackFrame) - (num_locals) * 8);

#define SAVE_STATE() \
  vms->heap.top = heap_top; \
  vms->heap.current_stack = current_stack; \
  stk->stack_pointer = stack_pointer;

#define RESTORE_STATE() \
  heap_top = vms->heap.top; \
  heap_limit = vms->heap.limit; \
  current_stack = vms->heap.current_stack; \
  stk = untag_stack(current_stack); \
  stack_pointer = stk->stack_pointer; \
  stack_limit = (char*)(stk->frames) + stk->size;

#define INT_TAG_BITS 0
#define REF_TAG_BITS 1
#define MARKER_TAG_BITS 2
#define BYTE_TAG_BITS 3
#define CHAR_TAG_BITS 4
#define FLOAT_TAG_BITS 5

#define FALSE_TYPE 0
#define TRUE_TYPE 1
#define BYTE_TYPE 2
#define CHAR_TYPE 3
#define INT_TYPE 4
#define FLOAT_TYPE 5
#define STACK_TYPE 6
#define FN_TYPE 7
#define TYPE_TYPE 8
#define LIVENESS_TRACKER_TYPE 9

#define EXTEND_HEAP_FN 0
#define EXTEND_STACK_FN 1
#define INIT_CONSTS_FN 2
#define EXECUTE_TOPLEVEL_COMMAND_FN 3

#define BOOLREF(x) (((x) << 3) + MARKER_TAG_BITS)

#define SYSTEM_RETURN_STUB -2

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

enum {
  LOG_BITS_IN_BYTE = 3,
  LOG_BYTES_IN_LONG = 3,
  LOG_BITS_IN_LONG = LOG_BYTES_IN_LONG + LOG_BITS_IN_BYTE,
  BYTES_IN_LONG  = 1 << LOG_BYTES_IN_LONG,
  BITS_IN_LONG = 1 << LOG_BITS_IN_LONG
};

static inline uint64_t bit_index (const void* p) {
  return ((uint64_t)p) >> LOG_BYTES_IN_LONG;
}
static inline uint64_t bit_shift (const void* p) {
  return bit_index(p) & (BITS_IN_LONG - 1);
}
static inline uint64_t bit_mask (const void* p) {
  return 1ULL << bit_shift(p);
}
static inline uint64_t* bit_address (const void* p, uint64_t* bitset_base) {
  return bitset_base + (bit_index(p) >> LOG_BITS_IN_LONG);
}
static inline void set_mark (const void* p, uint64_t* bitset_base) {
  *bit_address(p, bitset_base) |= bit_mask(p);
}
static inline void clear_mark (const void* p, uint64_t* bitset_base) {
  *bit_address(p, bitset_base) &= ~bit_mask(p);
}
static inline void set_bit (uint64_t bit_index, uint64_t* bitset_base) {
  uint64_t word_index = bit_index >> 6;
  uint64_t word_bit_index = bit_index & 63;
  uint64_t mask = 1ULL << word_bit_index;
  bitset_base[word_index] |= mask;
}
static inline void clear_bit (uint64_t bit_index, uint64_t* bitset_base) {
  uint64_t word_index = bit_index >> 6;
  uint64_t word_bit_index = bit_index & 63;
  uint64_t mask = 1ULL << word_bit_index;
  bitset_base[word_index] &= ~mask;
}
static inline uint64_t test_bit (uint64_t bit_index, uint64_t* bitset_base) {
  uint64_t word_index = bit_index >> 6;
  uint64_t word_bit_index = bit_index & 63;
  return (bitset_base[word_index] >> word_bit_index) & 1ULL;
}
static inline uint64_t test_and_set_bit (uint64_t bit_index, uint64_t* bitset_base) {
  uint64_t word_index = bit_index >> 6;
  uint64_t word_bit_index = bit_index & 63;
  uint64_t mask = 1ULL << word_bit_index;
  uint64_t old_value = bitset_base[word_index];
  bitset_base[word_index] = old_value | mask;
  return (old_value >> word_bit_index) & 1ULL;
}
static inline uint64_t test_and_clear_bit (uint64_t bit_index, uint64_t* bitset_base) {
  uint64_t word_index = bit_index >> 6;
  uint64_t word_bit_index = bit_index & 63;
  uint64_t mask = 1ULL << word_bit_index;
  uint64_t old_value = bitset_base[word_index];
  bitset_base[word_index] = old_value & ~mask;
  return (old_value >> word_bit_index) & 1ULL;
}

//============================================================
//========================= TRAPS ============================
//============================================================

int call_garbage_collector (VMState* vms, uint64_t total_size);
void call_print_stack_trace (VMState* vms, uint64_t stack);
void* call_collect_stack_trace (VMState* vms, uint64_t stack);
void c_trampoline (void* fptr, void* argbuffer, void* retbuffer);
uint64_t lowest_zero_bit_count (uint64_t x);

//============================================================
//=================== Forward Declarations ===================
//============================================================
int read_dispatch_table (VMState* vms, int format);

//============================================================
//==================== Write Barrier =========================
//============================================================

//Store the value, update the remembered set accordingly.
static inline void barriered_store (const VMState* vms, uint64_t* address, uint64_t value) {
  // First store the value
  *address = value;
  set_mark(address, vms->heap.bitset_base);
}

//============================================================
//===================== MAIN LOOP ============================
//============================================================

Stack* untag_stack (uint64_t current_stack){
  return (Stack*)(current_stack - 1 + 8);
}

uint64_t ptr_to_ref (void* p){
  return (uint64_t)p + REF_TAG_BITS;
}

//long iprint_start;
//long iprint_end;
//long iprint_step;
//long icounter;
//int iprint_init = 0;
//void init_iprint () {
//  if(!iprint_init){
//    iprint_init = 1;
//    icounter = 0;
//    FILE* file = fopen("iprint.txt", "r");
//    int n = fscanf(file, "%ld %ld %ld", &iprint_start, &iprint_end, &iprint_step);
//    if(n != 3){
//      printf("Couldn't read iprint.txt.");
//      exit(-1);
//    }
//  }
//}

void vmloop (VMState* vms, uint64_t stanza_crsp, int64_t starting_fid){
  //Pull out local cache
  char* instructions = vms->instructions;
  uint64_t* registers = vms->registers;
  uint64_t* global_offsets = vms->global_offsets;
  char* global_mem = vms->global_mem;
  uint64_t* const_table = vms->const_table;
  char* const_mem = vms->const_mem;
  uint32_t* data_offsets = vms->data_offsets;
  char* data_mem = vms->data_mem;
  uint64_t* code_offsets = vms->code_offsets;
  //Variable State
  //Changes in_between each boundary change
  char* heap_top = vms->heap.top;
  char* heap_limit = vms->heap.limit;
  uint64_t current_stack = vms->heap.current_stack;
  Stack* stk = untag_stack(current_stack);
  StackFrame* stack_pointer = stk->stack_pointer;
  char* stack_limit = (char*)(stk->frames) + stk->size;
  char* pc = instructions + code_offsets[starting_fid];

  //Timing
  //uint64_t* timings = (uint64_t*)malloc(255 * sizeof(uint64_t));
  //for(int i=0; i<255; i++) timings[i] = 0;
  //int last_opcode = -1;
  //uint64_t last_time;

  //Debug
  //init_iprint();

  //Repl Loop
  while(1){
    //icounter++;
    //int iprint = icounter >= iprint_start && icounter <= iprint_end && icounter % iprint_step == 0;

    //Save pre-decode PC because jump offsets are relative to
    //pre-decode PC.
    char* pc0 = pc;
    uint32_t W1 = PC_INT();
    int opcode = W1 & 0xFF;

    //uint64_t curtime = current_time_ms();
    //if(last_opcode >= 0)
    //  timings[last_opcode] += curtime - last_time;
    //last_opcode = opcode;
    //last_time = curtime;

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
      SET_LOCAL(y, (int64_t)(int32_t)value);
      continue;
    }
    case SET_OPCODE_CODE : {
      DECODE_C();
      SET_LOCAL(y, value);
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
      char* address = data_mem + 8 * data_offsets[value];
      SET_LOCAL(y, (uint64_t)address);
      continue;
    }
    case SET_OPCODE_CONST : {
      DECODE_C();
      SET_LOCAL(y, const_table[value]);
      continue;
    }
    case SET_OPCODE_WIDE : {
      DECODE_D();
      SET_LOCAL(x, value);
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
      SET_REG(y, (int64_t)(int32_t)value);
      continue;
    }
    case SET_REG_OPCODE_CODE : {
      DECODE_C();
      SET_REG(y, value);
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
      char* address = data_mem + 8 * data_offsets[value];
      SET_REG(y, (uint64_t)address);
      continue;
    }
    case SET_REG_OPCODE_CONST : {
      DECODE_C();
      SET_REG(y, const_table[value]);
      continue;
    }
    case SET_REG_OPCODE_WIDE : {
      DECODE_D();
      SET_REG(x, value);
      continue;
    }
    case GET_REG_OPCODE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, registers[value]);
      continue;
    }
    case CALL_OPCODE_LOCAL : {
      DECODE_C();
      int num_locals = y;
      uint64_t fid = LOCAL(value);
      uint64_t fpos = code_offsets[fid];
      PUSH_FRAME(num_locals);
      pc = instructions + fpos;
      continue;
    }
    case CALL_OPCODE_CODE : {
      DECODE_C();
      int num_locals = y;
      uint64_t fid = value;
      uint64_t fpos = code_offsets[fid];
      PUSH_FRAME(num_locals);
      pc = instructions + fpos;
      continue;
    }
    case CALL_CLOSURE_OPCODE : {
      DECODE_C();
      int num_locals = y;
      Function* clo = (Function*)(LOCAL(value) - REF_TAG_BITS + 8);
      uint64_t fid = clo->code;
      uint64_t fpos = code_offsets[fid];
      PUSH_FRAME(num_locals);
      pc = instructions + fpos;
      continue;
    }
    case TCALL_OPCODE_LOCAL : {
      DECODE_C();
      int num_locals = y;
      uint64_t fid = LOCAL(value);
      uint64_t fpos = code_offsets[fid];
      pc = instructions + fpos;
      continue;
    }
    case TCALL_OPCODE_CODE : {
      DECODE_C();
      int num_locals = y;
      uint64_t fid = value;
      uint64_t fpos = code_offsets[fid];
      pc = instructions + fpos;
      continue;
    }
    case TCALL_CLOSURE_OPCODE : {
      DECODE_A_UNSIGNED();
      Function* clo = (Function*)(LOCAL(value) - REF_TAG_BITS + 8);
      uint64_t fid = clo->code;
      uint64_t fpos = code_offsets[fid];
      pc = instructions + fpos;
      continue;
    }
    case CALLC_OPCODE_LOCAL : {
      DECODE_C();
      void* faddr = (void*)LOCAL(value);
      int num_locals = y;
      PUSH_FRAME(num_locals);
      SAVE_STATE();
      c_trampoline(faddr, registers, registers);
      RESTORE_STATE();
      pc = instructions + stack_pointer->returnpc;
      POP_FRAME(num_locals);
      continue;
    }
    case CALLC_OPCODE_WIDE : {
      DECODE_D();
      void* faddr = (void*)(uint64_t)value;
      int num_locals = x;
      PUSH_FRAME(num_locals);
      SAVE_STATE();
      c_trampoline(faddr, registers, registers);
      RESTORE_STATE();
      pc = instructions + stack_pointer->returnpc;
      POP_FRAME(num_locals);
      continue;
    }
    case POP_FRAME_OPCODE : {
      DECODE_A_UNSIGNED();
      int num_locals = value;
      POP_FRAME(num_locals);
      continue;
    }
    case LIVE_OPCODE : {
      DECODE_A_UNSIGNED();
      stack_pointer->liveness_map = value;
      continue;
    }
    case ENTER_STACK_OPCODE : {
      DECODE_A_UNSIGNED();
      //Save current stack
      stk->stack_pointer = stack_pointer;
      stk->pc = pc - instructions;
      //Load next stack
      current_stack = LOCAL(value);
      stk = untag_stack(current_stack);
      stack_pointer = stk->frames;
      stack_limit = (char*)(stk->frames) + stk->size;
      //Load starting address
      uint64_t fid = stk->pc;
      uint64_t stk_pc = code_offsets[fid];
      pc = instructions + stk_pc;
      continue;
    }
    case YIELD_OPCODE : {
      DECODE_A_UNSIGNED();
      //Save current stack
      stk->stack_pointer = stack_pointer;
      stk->pc = pc - instructions;
      //Load next stack
      current_stack = LOCAL(value);
      stk = untag_stack(current_stack);
      stack_pointer = stk->stack_pointer;
      stack_limit = (char*)(stk->frames) + stk->size;
      pc = instructions + stk->pc;
      continue;
    }
    case RETURN_OPCODE : {
      DECODE_A_UNSIGNED();
      int64_t retpc = stack_pointer->returnpc;
      if(retpc == SYSTEM_RETURN_STUB){
        //System stack no longer needed
        stk->stack_pointer = 0;
        //Swap stack and registers
        vms->heap.current_stack = vms->heap.system_stack;
        vms->heap.system_stack = current_stack;
        vms->registers = vms->system_registers;
        vms->system_registers = registers;
        //Restore stack state
        current_stack = vms->heap.current_stack;
        stk = untag_stack(current_stack);
        stack_pointer = stk->stack_pointer;
        stack_limit = (char*)(stk->frames) + stk->size;
        registers = vms->registers;
        //Continue where we were
        retpc = stk->pc;

        pc = instructions + retpc;
        continue;
      }
      else if(retpc < 0){
        //Save registers
        SAVE_STATE();
        //for(int i=0; i<255; i++)
        //  printf("Time of opcode %d = %llu\n", i, timings[i]);
        return;
      }
      else{
        pc = instructions + retpc;
        continue;
      }
    }
    case DUMP_OPCODE : {
      DECODE_A_UNSIGNED();
      int64_t xl = (int64_t)LOCAL(value);
      char xb = (char)xl;
      int xi = (int)xl;
      float xf = LOCAL_FLOAT(value);
      float xd = LOCAL_DOUBLE(value);
      printf("DUMP LOCAL %d: (byte = %d, int = %d, long = %" PRId64 ", ptr = %p, float = %f, double = %f)\n",
             value, xb, xi, xl, (void*)xl, xf, xd);
      continue;
    }
    case INT_ADD_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) + (int64_t)(LOCAL(value)));
      continue;
    }
    case INT_SUB_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) - (int64_t)(LOCAL(value)));
      continue;
    }
    case INT_MUL_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, ((int64_t)(LOCAL(y)) >> 32LL) * (int64_t)(LOCAL(value)));
      continue;
    }
    case INT_DIV_OPCODE : {
      DECODE_C();
      int64_t sy = (int64_t)LOCAL(y);
      int64_t sz = (int64_t)LOCAL(value);
      SET_LOCAL(x, (sy / sz) << 32LL);
      continue;
    }
    case INT_MOD_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) % (int64_t)(LOCAL(value)));
      continue;
    }
    case INT_AND_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) & (int64_t)(LOCAL(value)));
      continue;
    }
    case INT_OR_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) | (int64_t)(LOCAL(value)));
      continue;
    }
    case INT_XOR_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) ^ (int64_t)(LOCAL(value)));
      continue;
    }
    case INT_SHL_OPCODE : {
      DECODE_C();
      int64_t sy = (int64_t)LOCAL(y);
      int64_t sz = (int64_t)LOCAL(value);
      SET_LOCAL(x, sy << (sz >> 32LL));
      continue;
    }
    case INT_SHR_OPCODE : {
      DECODE_C();
      uint64_t uy = LOCAL(y);
      int64_t sz = (int64_t)LOCAL(value);
      uint64_t r = uy >> (sz >> 32LL);
      SET_LOCAL(x, (r >> 32LL) << 32LL);
      continue;
    }
    case INT_ASHR_OPCODE : {
      DECODE_C();
      int64_t sy = (int64_t)LOCAL(y);
      int64_t sz = (int64_t)LOCAL(value);
      uint64_t r = sy >> (sz >> 32LL);
      SET_LOCAL(x, (r >> 32LL) << 32LL);
      continue;
    }
    case INT_LT_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, BOOLREF((int64_t)(LOCAL(y)) < (int64_t)(LOCAL(value))));
      continue;
    }
    case INT_GT_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, BOOLREF((int64_t)(LOCAL(y)) > (int64_t)(LOCAL(value))));
      continue;
    }
    case INT_LE_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, BOOLREF((int64_t)(LOCAL(y)) <= (int64_t)(LOCAL(value))));
      continue;
    }
    case INT_GE_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, BOOLREF((int64_t)(LOCAL(y)) >= (int64_t)(LOCAL(value))));
      continue;
    }
    case REF_EQ_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, BOOLREF(LOCAL(y) == LOCAL(value)));
      continue;
    }
    case EQ_OPCODE_REF : {
      DECODE_C();
      SET_LOCAL(x, LOCAL(y) == LOCAL(value));
      continue;
    }
    case EQ_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (uint8_t)LOCAL(y) == (uint8_t)LOCAL(value));
      continue;
    }
    case EQ_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)LOCAL(y) == (int32_t)LOCAL(value));
      continue;
    }
    case EQ_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)LOCAL(y) == (int64_t)LOCAL(value));
      continue;
    }
    case EQ_OPCODE_FLOAT : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_FLOAT(y) == LOCAL_FLOAT(value));
      continue;
    }
    case EQ_OPCODE_DOUBLE : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_DOUBLE(y) == LOCAL_DOUBLE(value));
      continue;
    }
    case REF_NE_OPCODE : {
      DECODE_C();
      SET_LOCAL(x, BOOLREF(LOCAL(y) != LOCAL(value)));
      continue;
    }
    case NE_OPCODE_REF : {
      DECODE_C();
      SET_LOCAL(x, LOCAL(y) != LOCAL(value));
      continue;
    }
    case NE_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (uint8_t)LOCAL(y) != (uint8_t)LOCAL(value));
      continue;
    }
    case NE_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)LOCAL(y) != (int32_t)LOCAL(value));
      continue;
    }
    case NE_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)LOCAL(y) != (int64_t)LOCAL(value));
      continue;
    }
    case NE_OPCODE_FLOAT : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_FLOAT(y) != LOCAL_FLOAT(value));
      continue;
    }
    case NE_OPCODE_DOUBLE : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_DOUBLE(y) != LOCAL_DOUBLE(value));
      continue;
    }
    case ADD_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (char)(LOCAL(y)) + (char)(LOCAL(value)));
      continue;
    }
    case ADD_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) + (int32_t)(LOCAL(value)));
      continue;
    }
    case ADD_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) + (int64_t)(LOCAL(value)));
      continue;
    }
    case ADD_OPCODE_FLOAT : {
      DECODE_C();
      SET_LOCAL_FLOAT(x, LOCAL_FLOAT(y) + LOCAL_FLOAT(value));
      continue;
    }
    case ADD_OPCODE_DOUBLE : {
      DECODE_C();
      SET_LOCAL_DOUBLE(x, LOCAL_DOUBLE(y) + LOCAL_DOUBLE(value));
      continue;
    }
    case SUB_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (char)(LOCAL(y)) - (char)(LOCAL(value)));
      continue;
    }
    case SUB_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) - (int32_t)(LOCAL(value)));
      continue;
    }
    case SUB_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) - (int64_t)(LOCAL(value)));
      continue;
    }
    case SUB_OPCODE_FLOAT : {
      DECODE_C();
      SET_LOCAL_FLOAT(x, LOCAL_FLOAT(y) - LOCAL_FLOAT(value));
      continue;
    }
    case SUB_OPCODE_DOUBLE : {
      DECODE_C();
      SET_LOCAL_DOUBLE(x, LOCAL_DOUBLE(y) - LOCAL_DOUBLE(value));
      continue;
    }
    case MUL_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (char)(LOCAL(y)) * (char)(LOCAL(value)));
      continue;
    }
    case MUL_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) * (int32_t)(LOCAL(value)));
      continue;
    }
    case MUL_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) * (int64_t)(LOCAL(value)));
      continue;
    }
    case MUL_OPCODE_FLOAT : {
      DECODE_C();
      SET_LOCAL_FLOAT(x, LOCAL_FLOAT(y) * LOCAL_FLOAT(value));
      continue;
    }
    case MUL_OPCODE_DOUBLE : {
      DECODE_C();
      SET_LOCAL_DOUBLE(x, LOCAL_DOUBLE(y) * LOCAL_DOUBLE(value));
      continue;
    }
    case DIV_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (char)(LOCAL(y)) / (char)(LOCAL(value)));
      continue;
    }
    case DIV_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) / (int32_t)(LOCAL(value)));
      continue;
    }
    case DIV_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) / (int64_t)(LOCAL(value)));
      continue;
    }
    case DIV_OPCODE_FLOAT : {
      DECODE_C();
      SET_LOCAL_FLOAT(x, LOCAL_FLOAT(y) / LOCAL_FLOAT(value));
      continue;
    }
    case DIV_OPCODE_DOUBLE : {
      DECODE_C();
      SET_LOCAL_DOUBLE(x, LOCAL_DOUBLE(y) / LOCAL_DOUBLE(value));
      continue;
    }
    case MOD_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (char)(LOCAL(y)) % (char)(LOCAL(value)));
      continue;
    }
    case MOD_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) % (int32_t)(LOCAL(value)));
      continue;
    }
    case MOD_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) % (int64_t)(LOCAL(value)));
      continue;
    }
    case AND_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (char)(LOCAL(y)) & (char)(LOCAL(value)));
      continue;
    }
    case AND_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) & (int32_t)(LOCAL(value)));
      continue;
    }
    case AND_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) & (int64_t)(LOCAL(value)));
      continue;
    }
    case OR_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (char)(LOCAL(y)) | (char)(LOCAL(value)));
      continue;
    }
    case OR_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) | (int32_t)(LOCAL(value)));
      continue;
    }
    case OR_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) | (int64_t)(LOCAL(value)));
      continue;
    }
    case XOR_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (char)(LOCAL(y)) ^ (char)(LOCAL(value)));
      continue;
    }
    case XOR_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) ^ (int32_t)(LOCAL(value)));
      continue;
    }
    case XOR_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) ^ (int64_t)(LOCAL(value)));
      continue;
    }
    case SHL_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (char)(LOCAL(y)) << (char)(LOCAL(value)));
      continue;
    }
    case SHL_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) << (int32_t)(LOCAL(value)));
      continue;
    }
    case SHL_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) << (int64_t)(LOCAL(value)));
      continue;
    }
    case SHR_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (unsigned char)(LOCAL(y)) >> (unsigned char)(LOCAL(value)));
      continue;
    }
    case SHR_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (uint32_t)(LOCAL(y)) >> (uint32_t)(LOCAL(value)));
      continue;
    }
    case SHR_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (uint64_t)(LOCAL(y)) >> (uint64_t)(LOCAL(value)));
      continue;
    }
    case ASHR_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) >> (int32_t)(LOCAL(value)));
      continue;
    }
    case ASHR_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) >> (int64_t)(LOCAL(value)));
      continue;
    }
    case LT_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) < (int32_t)(LOCAL(value)));
      continue;
    }
    case LT_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) < (int64_t)(LOCAL(value)));
      continue;
    }
    case LT_OPCODE_FLOAT : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_FLOAT(y) < LOCAL_FLOAT(value));
      continue;
    }
    case LT_OPCODE_DOUBLE : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_DOUBLE(y) < LOCAL_DOUBLE(value));
      continue;
    }
    case GT_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) > (int32_t)(LOCAL(value)));
      continue;
    }
    case GT_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) > (int64_t)(LOCAL(value)));
      continue;
    }
    case GT_OPCODE_FLOAT : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_FLOAT(y) > LOCAL_FLOAT(value));
      continue;
    }
    case GT_OPCODE_DOUBLE : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_DOUBLE(y) > LOCAL_DOUBLE(value));
      continue;
    }
    case LE_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) <= (int32_t)(LOCAL(value)));
      continue;
    }
    case LE_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) <= (int64_t)(LOCAL(value)));
      continue;
    }
    case LE_OPCODE_FLOAT : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_FLOAT(y) <= LOCAL_FLOAT(value));
      continue;
    }
    case LE_OPCODE_DOUBLE : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_DOUBLE(y) <= LOCAL_DOUBLE(value));
      continue;
    }
    case GE_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (int32_t)(LOCAL(y)) >= (int32_t)(LOCAL(value)));
      continue;
    }
    case GE_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (int64_t)(LOCAL(y)) >= (int64_t)(LOCAL(value)));
      continue;
    }
    case GE_OPCODE_FLOAT : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_FLOAT(y) >= LOCAL_FLOAT(value));
      continue;
    }
    case GE_OPCODE_DOUBLE : {
      DECODE_C();
      SET_LOCAL(x, LOCAL_DOUBLE(y) >= LOCAL_DOUBLE(value));
      continue;
    }

    case ULE_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (uint8_t)(LOCAL(y)) <= (uint8_t)(LOCAL(value)));
      continue;
    }
    case ULE_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (uint32_t)(LOCAL(y)) <= (uint32_t)(LOCAL(value)));
      continue;
    }
    case ULE_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (uint64_t)(LOCAL(y)) <= (uint64_t)(LOCAL(value)));
      continue;
    }
    case ULT_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (uint8_t)(LOCAL(y)) < (uint8_t)(LOCAL(value)));
      continue;
    }
    case ULT_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (uint32_t)(LOCAL(y)) < (uint32_t)(LOCAL(value)));
      continue;
    }
    case ULT_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (uint64_t)(LOCAL(y)) < (uint64_t)(LOCAL(value)));
      continue;
    }
    case UGT_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (uint8_t)(LOCAL(y)) > (uint8_t)(LOCAL(value)));
      continue;
    }
    case UGT_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (uint32_t)(LOCAL(y)) > (uint32_t)(LOCAL(value)));
      continue;
    }
    case UGT_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (uint64_t)(LOCAL(y)) > (uint64_t)(LOCAL(value)));
      continue;
    }
    case UGE_OPCODE_BYTE : {
      DECODE_C();
      SET_LOCAL(x, (uint8_t)(LOCAL(y)) >= (uint8_t)(LOCAL(value)));
      continue;
    }
    case UGE_OPCODE_INT : {
      DECODE_C();
      SET_LOCAL(x, (uint32_t)(LOCAL(y)) >= (uint32_t)(LOCAL(value)));
      continue;
    }
    case UGE_OPCODE_LONG : {
      DECODE_C();
      SET_LOCAL(x, (uint64_t)(LOCAL(y)) >= (uint64_t)(LOCAL(value)));
      continue;
    }
    case INT_NOT_OPCODE : {
      DECODE_B_UNSIGNED();
      uint64_t y = LOCAL(value);
      SET_LOCAL(x, ((~ y) >> 32LL) << 32LL);
      continue;
    }
    case INT_NEG_OPCODE : {
      DECODE_B_UNSIGNED();
      int64_t y = LOCAL(value);
      SET_LOCAL(x, - y);
      continue;
    }
    case NOT_OPCODE_BYTE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, ~ ((uint8_t)LOCAL(value)));
      continue;
    }
    case NOT_OPCODE_INT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, ~ ((uint32_t)LOCAL(value)));
      continue;
    }
    case NOT_OPCODE_LONG : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, ~ ((uint64_t)LOCAL(value)));
      continue;
    }
    case NEG_OPCODE_INT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, - ((int32_t)LOCAL(value)));
      continue;
    }
    case NEG_OPCODE_LONG : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, - ((int64_t)LOCAL(value)));
      continue;
    }
    case NEG_OPCODE_FLOAT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL_FLOAT(x, - LOCAL_FLOAT(value));
      continue;
    }
    case NEG_OPCODE_DOUBLE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL_DOUBLE(x, - LOCAL_DOUBLE(value));
      continue;
    }
    case DEREF_OPCODE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, LOCAL(value) + 8 - REF_TAG_BITS);
      continue;
    }
    case TYPEOF_OPCODE : {
      DECODE_C();
      int format = value;
      int index = read_dispatch_table(vms, format);
      SET_LOCAL(x, index);
      continue;
    }
    case JUMP_SET_OPCODE : {
      DECODE_F();
      F_JUMP(LOCAL(x));
    }
    case JUMP_TAGBITS_OPCODE : {
      DECODE_F();
      int tagbits = (int)(LOCAL(x)) & 0x7;
      int bits = y;
      F_JUMP(tagbits == bits);
    }
    case JUMP_TAGWORD_OPCODE : {
      DECODE_F();
      uint64_t obj = LOCAL(x);
      int tagbits = (int)obj & 0x7;
      int tag = LOCAL(y);
      if(tagbits == 1){
        int* p = (int*)(obj - 1);
        F_JUMP(*p == tag);
      }else{
        pc = pc0 + (n2 * 4);
        continue;
      }
    }
    case GOTO_OPCODE : {
      DECODE_A_SIGNED();
      pc = pc0 + (value * 4);
      continue;
    }
    case CONV_OPCODE_BYTE_FLOAT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, (uint8_t)(LOCAL_FLOAT(value)));
      continue;
    }
    case CONV_OPCODE_BYTE_DOUBLE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, (uint8_t)(LOCAL_DOUBLE(value)));
      continue;
    }
    case CONV_OPCODE_INT_BYTE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, (int32_t)(uint8_t)(LOCAL(value)));
      continue;
    }
    case CONV_OPCODE_INT_FLOAT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, (int32_t)(LOCAL_FLOAT(value)));
      continue;
    }
    case CONV_OPCODE_INT_DOUBLE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, (int32_t)(LOCAL_DOUBLE(value)));
      continue;
    }
    case CONV_OPCODE_LONG_BYTE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, (int64_t)(uint8_t)(LOCAL(value)));
      continue;
    }
    case CONV_OPCODE_LONG_INT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, (int64_t)(int32_t)(LOCAL(value)));
      continue;
    }
    case CONV_OPCODE_LONG_FLOAT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, (int64_t)(LOCAL_FLOAT(value)));
      continue;
    }
    case CONV_OPCODE_LONG_DOUBLE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, (int64_t)(LOCAL_DOUBLE(value)));
      continue;
    }
    case CONV_OPCODE_FLOAT_BYTE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL_FLOAT(x, (uint8_t)(LOCAL(value)));
      continue;
    }
    case CONV_OPCODE_FLOAT_INT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL_FLOAT(x, (int32_t)(LOCAL(value)));
      continue;
    }
    case CONV_OPCODE_FLOAT_LONG : {
      DECODE_B_UNSIGNED();
      SET_LOCAL_FLOAT(x, (int64_t)(LOCAL(value)));
      continue;
    }
    case CONV_OPCODE_FLOAT_DOUBLE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL_FLOAT(x, LOCAL_DOUBLE(value));
      continue;
    }
    case CONV_OPCODE_DOUBLE_BYTE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL_DOUBLE(x, (uint8_t)(LOCAL(value)));
      continue;
    }
    case CONV_OPCODE_DOUBLE_INT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL_DOUBLE(x, (int32_t)(LOCAL(value)));
      continue;
    }
    case CONV_OPCODE_DOUBLE_LONG : {
      DECODE_B_UNSIGNED();
      SET_LOCAL_DOUBLE(x, (int64_t)(LOCAL(value)));
      continue;
    }
    case CONV_OPCODE_DOUBLE_FLOAT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL_DOUBLE(x, LOCAL_FLOAT(value));
      continue;
    }
    case DETAG_OPCODE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, LOCAL(value) >> 32LL);
      continue;
    }
    case TAG_OPCODE_BYTE : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, ((uint64_t)(uint8_t)(LOCAL(value)) << 32LL) + BYTE_TAG_BITS);
      continue;
    }
    case TAG_OPCODE_CHAR : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, ((uint64_t)(uint8_t)(LOCAL(value)) << 32LL) + CHAR_TAG_BITS);
      continue;
    }
    case TAG_OPCODE_INT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, ((uint64_t)LOCAL(value) << 32LL) + INT_TAG_BITS);
      continue;
    }
    case TAG_OPCODE_FLOAT : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, ((uint64_t)LOCAL(value) << 32LL) + FLOAT_TAG_BITS);
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
    case STORE_WITH_BARRIER_OPCODE : {
      DECODE_E();

      //Retrieve address to store to and value to store.
      uint64_t* address = (uint64_t*)(LOCAL(x) + value);
      uint64_t val = (uint64_t)(LOCAL(z));
      barriered_store(vms, address, val);
      continue;
    }
    case STORE_WITH_BARRIER_OPCODE_VAR_OFFSET : {
      DECODE_E();

      //Retrieve address to store to and value to store.
      uint64_t* address = (uint64_t*)(LOCAL(x) + LOCAL(y) + value);
      uint64_t val = (uint64_t)(LOCAL(z));
      barriered_store(vms, address, val);
      continue;
    }
    case LOAD_OPCODE_1 : {
      DECODE_E();
      char* address = (char*)(LOCAL(y) + value);
      SET_LOCAL(x, *address);
      continue;
    }
    case LOAD_OPCODE_4 : {
      DECODE_E();
      int32_t* address = (int32_t*)(LOCAL(y) + value);
      SET_LOCAL(x, *address);
      continue;
    }
    case LOAD_OPCODE_8 : {
      DECODE_E();
      int64_t* address = (int64_t*)(LOCAL(y) + value);
      SET_LOCAL(x, *address);
      continue;
    }
    case LOAD_OPCODE_1_VAR_OFFSET : {
      DECODE_E();
      char* address = (char*)(LOCAL(y) + LOCAL(z) + value);
      SET_LOCAL(x, *address);
      continue;
    }
    case LOAD_OPCODE_4_VAR_OFFSET : {
      DECODE_E();
      int32_t* address = (int32_t*)(LOCAL(y) + LOCAL(z) + value);
      SET_LOCAL(x, *address);
      continue;
    }
    case LOAD_OPCODE_8_VAR_OFFSET : {
      DECODE_E();
      int64_t* address = (int64_t*)(LOCAL(y) + LOCAL(z) + value);
      SET_LOCAL(x, *address);
      continue;
    }
    case RESERVE_OPCODE_LOCAL : {
      DECODE_C();
      uint64_t size = 8 + LOCAL(value);
      size = (size + 7) & -8;
      int num_locals = y;
      int offset = x * 4;
      if(heap_top + size <= heap_limit){
        pc = pc0 + offset;
        continue;
      }else{
        SET_REG(0, BOOLREF(0));
        SET_REG(1, 1ULL);
        SET_REG(2, size);
        uint64_t fpos = code_offsets[EXTEND_HEAP_FN];
        PUSH_FRAME(num_locals);
        pc = instructions + fpos;
        continue;
      }
    }
    case RESERVE_OPCODE_CONST : {
      DECODE_C();
      uint64_t size = value;
      int num_locals = y;
      int offset = x * 4;
      if(heap_top + size <= heap_limit){
        pc = pc0 + offset;
        continue;
      }else{
        SET_REG(0, BOOLREF(0));
        SET_REG(1, 1ULL);
        SET_REG(2, size);
        uint64_t fpos = code_offsets[EXTEND_HEAP_FN];
        PUSH_FRAME(num_locals);
        pc = instructions + fpos;
        continue;
      }
    }
    case ALLOC_OPCODE_CONST : {
      DECODE_C();
      int num_bytes = 8 + y;
      int type = value;
      *(uint64_t*)heap_top = type;
      uint64_t obj = ptr_to_ref(heap_top);
      SET_LOCAL(x, obj);
      heap_top = heap_top + num_bytes;
      continue;
    }
    case ALLOC_OPCODE_LOCAL : {
      DECODE_C();
      uint64_t num_bytes = 8 + LOCAL(y);
      num_bytes = (num_bytes + 7) & -8;
      int type = value;
      *(uint64_t*)heap_top = type;
      uint64_t obj = ptr_to_ref(heap_top);
      SET_LOCAL(x, obj);
      heap_top = heap_top + num_bytes;
      continue;
    }
    case GC_OPCODE : {
      DECODE_B_UNSIGNED();
      //Size to extend
      uint64_t size = LOCAL(value);
      //Call GC
      SAVE_STATE();
      int64_t remaining = call_garbage_collector(vms, size);
      RESTORE_STATE();
      //Return heap remaining
      SET_LOCAL(x, remaining);
      continue;
    }
    case PRINT_STACK_TRACE_OPCODE : {
      DECODE_B_UNSIGNED();
      uint64_t stack = LOCAL(value);
      call_print_stack_trace(vms, stack);
      SET_LOCAL(x, 0);
      continue;
    }
    case COLLECT_STACK_TRACE_OPCODE : {
      DECODE_B_UNSIGNED();
      uint64_t stack = LOCAL(value);
      void* packed_trace = call_collect_stack_trace(vms, stack);
      SET_LOCAL(x, (uint64_t)packed_trace);
      continue;
    }
    case FLUSH_VM_OPCODE : {
      DECODE_A_UNSIGNED();
      SAVE_STATE();
      SET_LOCAL(value, (uint64_t)vms);
      continue;
    }
    case C_RSP_OPCODE : {
      DECODE_A_UNSIGNED();
      SET_LOCAL(value, stanza_crsp);
      continue;
    }
    case JUMP_INT_LT_OPCODE : {
      DECODE_F();
      F_JUMP((int64_t)LOCAL(x) < (int64_t)LOCAL(y));
    }
    case JUMP_INT_GT_OPCODE : {
      DECODE_F();
      F_JUMP((int64_t)LOCAL(x) > (int64_t)LOCAL(y));
    }
    case JUMP_INT_LE_OPCODE : {
      DECODE_F();
      F_JUMP((int64_t)LOCAL(x) <= (int64_t)LOCAL(y));
    }
    case JUMP_INT_GE_OPCODE : {
      DECODE_F();
      F_JUMP((int64_t)LOCAL(x) >= (int64_t)LOCAL(y));
    }
    case JUMP_EQ_OPCODE_REF : {
      DECODE_F();
      F_JUMP(LOCAL(x) == LOCAL(y));
    }
    case JUMP_EQ_OPCODE_BYTE : {
      DECODE_F();
      F_JUMP((int8_t)LOCAL(x) == (int8_t)LOCAL(y));
    }
    case JUMP_EQ_OPCODE_INT : {
      DECODE_F();
      F_JUMP((int32_t)LOCAL(x) == (int32_t)LOCAL(y));
    }
    case JUMP_EQ_OPCODE_LONG : {
      DECODE_F();
      F_JUMP((int64_t)LOCAL(x) == (int64_t)LOCAL(y));
    }
    case JUMP_EQ_OPCODE_FLOAT : {
      DECODE_F();
      F_JUMP(LOCAL_FLOAT(x) == LOCAL_FLOAT(y));
    }
    case JUMP_EQ_OPCODE_DOUBLE : {
      DECODE_F();
      F_JUMP(LOCAL_DOUBLE(x) == LOCAL_DOUBLE(y));
    }
    case JUMP_NE_OPCODE_REF : {
      DECODE_F();
      F_JUMP(LOCAL(x) != LOCAL(y));
    }
    case JUMP_NE_OPCODE_BYTE : {
      DECODE_F();
      F_JUMP((int8_t)LOCAL(x) != (int8_t)LOCAL(y));
    }
    case JUMP_NE_OPCODE_INT : {
      DECODE_F();
      F_JUMP((int32_t)LOCAL(x) != (int32_t)LOCAL(y));
    }
    case JUMP_NE_OPCODE_LONG : {
      DECODE_F();
      F_JUMP((int64_t)LOCAL(x) != (int64_t)LOCAL(y));
    }
    case JUMP_NE_OPCODE_FLOAT : {
      DECODE_F();
      F_JUMP(LOCAL_FLOAT(x) != LOCAL_FLOAT(y));
    }
    case JUMP_NE_OPCODE_DOUBLE : {
      DECODE_F();
      F_JUMP(LOCAL_DOUBLE(x) != LOCAL_DOUBLE(y));
    }
    case JUMP_LT_OPCODE_INT : {
      DECODE_F();
      F_JUMP((int32_t)LOCAL(x) < (int32_t)LOCAL(y));
    }
    case JUMP_LT_OPCODE_LONG : {
      DECODE_F();
      F_JUMP((int64_t)LOCAL(x) < (int64_t)LOCAL(y));
    }
    case JUMP_LT_OPCODE_FLOAT : {
      DECODE_F();
      F_JUMP(LOCAL_FLOAT(x) < LOCAL_FLOAT(y));
    }
    case JUMP_LT_OPCODE_DOUBLE : {
      DECODE_F();
      F_JUMP(LOCAL_DOUBLE(x) < LOCAL_DOUBLE(y));
    }
    case JUMP_GT_OPCODE_INT : {
      DECODE_F();
      F_JUMP((int32_t)LOCAL(x) > (int32_t)LOCAL(y));
    }
    case JUMP_GT_OPCODE_LONG : {
      DECODE_F();
      F_JUMP((int64_t)LOCAL(x) > (int64_t)LOCAL(y));
    }
    case JUMP_GT_OPCODE_FLOAT : {
      DECODE_F();
      F_JUMP(LOCAL_FLOAT(x) > LOCAL_FLOAT(y));
    }
    case JUMP_GT_OPCODE_DOUBLE : {
      DECODE_F();
      F_JUMP(LOCAL_DOUBLE(x) > LOCAL_DOUBLE(y));
    }
    case JUMP_LE_OPCODE_INT : {
      DECODE_F();
      F_JUMP((int32_t)LOCAL(x) <= (int32_t)LOCAL(y));
    }
    case JUMP_LE_OPCODE_LONG : {
      DECODE_F();
      F_JUMP((int64_t)LOCAL(x) <= (int64_t)LOCAL(y));
    }
    case JUMP_LE_OPCODE_FLOAT : {
      DECODE_F();
      F_JUMP(LOCAL_FLOAT(x) <= LOCAL_FLOAT(y));
    }
    case JUMP_LE_OPCODE_DOUBLE : {
      DECODE_F();
      F_JUMP(LOCAL_DOUBLE(x) <= LOCAL_DOUBLE(y));
    }
    case JUMP_GE_OPCODE_INT : {
      DECODE_F();
      F_JUMP((int32_t)LOCAL(x) >= (int32_t)LOCAL(y));
    }
    case JUMP_GE_OPCODE_LONG : {
      DECODE_F();
      F_JUMP((int64_t)LOCAL(x) >= (int64_t)LOCAL(y));
    }
    case JUMP_GE_OPCODE_FLOAT : {
      DECODE_F();
      F_JUMP(LOCAL_FLOAT(x) >= LOCAL_FLOAT(y));
    }
    case JUMP_GE_OPCODE_DOUBLE : {
      DECODE_F();
      F_JUMP(LOCAL_DOUBLE(x) >= LOCAL_DOUBLE(y));
    }
    case JUMP_ULE_OPCODE_BYTE : {
      DECODE_F();
      F_JUMP((uint8_t)LOCAL(x) <= (uint8_t)LOCAL(y));
    }
    case JUMP_ULE_OPCODE_INT : {
      DECODE_F();
      F_JUMP((uint32_t)LOCAL(x) <= (uint32_t)LOCAL(y));
    }
    case JUMP_ULE_OPCODE_LONG : {
      DECODE_F();
      F_JUMP((uint64_t)LOCAL(x) <= (uint64_t)LOCAL(y));
    }
    case JUMP_ULT_OPCODE_BYTE : {
      DECODE_F();
      F_JUMP((uint8_t)LOCAL(x) < (uint8_t)LOCAL(y));
    }
    case JUMP_ULT_OPCODE_INT : {
      DECODE_F();
      F_JUMP((uint32_t)LOCAL(x) < (uint32_t)LOCAL(y));
    }
    case JUMP_ULT_OPCODE_LONG : {
      DECODE_F();
      F_JUMP((uint64_t)LOCAL(x) < (uint64_t)LOCAL(y));
    }
    case JUMP_UGE_OPCODE_BYTE : {
      DECODE_F();
      F_JUMP((uint8_t)LOCAL(x) >= (uint8_t)LOCAL(y));
    }
    case JUMP_UGE_OPCODE_INT : {
      DECODE_F();
      F_JUMP((uint32_t)LOCAL(x) >= (uint32_t)LOCAL(y));
    }
    case JUMP_UGE_OPCODE_LONG : {
      DECODE_F();
      F_JUMP((uint64_t)LOCAL(x) >= (uint64_t)LOCAL(y));
    }
    case JUMP_UGT_OPCODE_BYTE : {
      DECODE_F();
      F_JUMP((uint8_t)LOCAL(x) > (uint8_t)LOCAL(y));
    }
    case JUMP_UGT_OPCODE_INT : {
      DECODE_F();
      F_JUMP((uint32_t)LOCAL(x) > (uint32_t)LOCAL(y));
    }
    case JUMP_UGT_OPCODE_LONG : {
      DECODE_F();
      F_JUMP((uint64_t)LOCAL(x) > (uint64_t)LOCAL(y));
    }
    case DISPATCH_OPCODE : {
      DECODE_A_UNSIGNED();
      uint32_t* tgts = (uint32_t*)(pc + 4);
      //DECODE_TGTS();
      int format = value;
      int index = read_dispatch_table(vms, format);
      int tgt = tgts[index];
      pc = pc0 + (tgt * 4);
      continue;
    }
    case DISPATCH_METHOD_OPCODE : {
      DECODE_A_UNSIGNED();
      uint32_t* tgts = (uint32_t*)(pc + 4);
      //DECODE_TGTS();
      int format = value;
      int index = read_dispatch_table(vms, format);
      if(index < 2){
        int tgt = tgts[index];
        pc = pc0 + (tgt * 4);
        continue;
      }else{
        int fid = index - 2;
        uint64_t fpos = code_offsets[fid];
        pc = instructions + fpos;
        continue;
      }
    }
    case JUMP_REG_OPCODE : {
      DECODE_C();
      int reg = x;
      uint64_t arity = y;
      int offset = value * 4;
      if(registers[reg] == arity){
        pc = pc0 + offset;
      }
      continue;
    }
    case FNENTRY_OPCODE : {
      DECODE_A_UNSIGNED();
      int frame_size = value * 8 + sizeof(StackFrame);
      int size_required = frame_size + sizeof(StackFrame);
      if((char*)stack_pointer + size_required > stack_limit){
        //Save current stack
        stk->stack_pointer = stack_pointer;
        stk->pc = pc - instructions;
        //Swap stack and registers
        vms->heap.current_stack = vms->heap.system_stack;
        vms->heap.system_stack = current_stack;
        vms->registers = vms->system_registers;
        vms->system_registers = registers;
        //Restore stack state
        current_stack = vms->heap.current_stack;
        stk = untag_stack(current_stack);
        stack_pointer = stk->stack_pointer;
        stack_limit = (char*)(stk->frames) + stk->size;
        registers = vms->registers;
        //Set arguments
        SET_REG(0, BOOLREF(0));
        SET_REG(1, 1ULL);
        SET_REG(2, size_required);
        stack_pointer = stk->frames;
        stack_pointer->returnpc = SYSTEM_RETURN_STUB;
        //Jump to stack extender
        uint64_t fpos = code_offsets[EXTEND_STACK_FN];
        pc = instructions + fpos;
      }
      continue;
    }
    case LOWEST_ZERO_BIT_COUNT_OPCODE_LONG : {
      DECODE_B_UNSIGNED();
      SET_LOCAL(x, lowest_zero_bit_count((uint64_t)LOCAL(value)));
      continue;
    }
    case SET_BIT_OPCODE : {
      DECODE_C();
      uint64_t bit_index = (uint64_t)LOCAL(y);
      uint64_t* bitset_base = (uint64_t*)LOCAL(value);
      set_bit(bit_index, bitset_base);
      continue;
    }
    case CLEAR_BIT_OPCODE : {
      DECODE_C();
      uint64_t bit_index = (uint64_t)LOCAL(y);
      uint64_t* bitset_base = (uint64_t*)LOCAL(value);
      clear_bit(bit_index, bitset_base);
      continue;
    }
    case TEST_BIT_OPCODE : {
      DECODE_C();
      uint64_t bit_index = (uint64_t)LOCAL(y);
      uint64_t* bitset_base = (uint64_t*)LOCAL(value);
      SET_LOCAL(x, test_bit(bit_index, bitset_base));
      continue;
    }
    case TEST_AND_SET_BIT_OPCODE : {
      DECODE_C();
      uint64_t bit_index = (uint64_t)LOCAL(y);
      uint64_t* bitset_base = (uint64_t*)LOCAL(value);
      SET_LOCAL(x, test_and_set_bit(bit_index, bitset_base));
      continue;
    }
    case TEST_AND_CLEAR_BIT_OPCODE : {
      DECODE_C();
      uint64_t bit_index = (uint64_t)LOCAL(y);
      uint64_t* bitset_base = (uint64_t*)LOCAL(value);
      SET_LOCAL(x, test_and_clear_bit(bit_index, bitset_base));
      continue;
    }
    }

    //Done
    printf("Invalid opcode: %d\n", opcode);
    exit(-1);
  }
}

//============================================================
//================= Dispatch Interpreter =====================
//============================================================

typedef struct {
  int index;
  int n;
} TrieTable;

typedef struct {
  int d0;
  int dtable[];
} DTable;

typedef struct {
  int key;
  int value;
} DKV;

int PRIM_TYPEIDS[] = {INT_TYPE, 0, 0, BYTE_TYPE, CHAR_TYPE, FLOAT_TYPE};

int argtype (VMState* vms, int i){
  uint64_t x = vms->registers[i];
  int tagbits = (int)x & 0x7;
  if(tagbits == REF_TAG_BITS){
    int* p = (int*)(x - 1);
    return *p;
  }
  else if(tagbits == MARKER_TAG_BITS){
    return (int)(x >> 3);
  }
  else{
    return PRIM_TYPEIDS[tagbits];
  }
}

DKV* small_etable (TrieTable* trie_table){
  void* p = trie_table;
  return p + sizeof(TrieTable);
}

DKV* big_etable (DTable* dtable, int n){
  void* p = dtable->dtable + n;
  return p;
}

DTable* trie_dtable (TrieTable* trie_table){
  void* p = trie_table;
  return p + sizeof(TrieTable);
}

int default_value (DKV* etable, int n){
  return etable[n].key;
}

int lookup_small_etable (DKV* etable, int t, int n){
  for(int i=0; i<n; i++){
    DKV e = etable[i];
    if(e.key == t) return e.value;
  }
  return default_value(etable,n);
}

int lookup_etable (DKV* etable, int slot, int t, int n){
  DKV e = etable[slot];
  if(e.key == t) return e.value;
  return default_value(etable,n);
}

int dhash (int d, int x, int n){
  uint32_t a = x;
  a = (a + 0x7ed55d16 + d) + (a << 12);
  a = (a ^ 0xc761c23c) ^ (a >> 19);
  a = (a + 0x165667b1) + (a << 5);
  a = (a + 0xd3a2646c) ^ (a << 9);
  a = (a + 0xfd7046c5) + (a << 3);
  a = (a ^ 0xb55a4f09) ^ (a >> 16);
  return ((int)a & 0x7FFFFFFF) % n;
}

int lookup_trie_table (VMState* vms, TrieTable* trie_table){
  int n = trie_table->n;
  int type = argtype(vms, trie_table->index);
  if(n <= 4){
    return lookup_small_etable(small_etable(trie_table), type, n);
  }else{
    DTable* dtable = trie_dtable(trie_table);
    int dslot = dhash(dtable->d0, type, n);
    int d = dtable->dtable[dslot];
    if(d == 0){
      return default_value(big_etable(dtable,n), n);
    }else{
      int slot = d < 0? -d - 1 : dhash(d, type, n);
      return lookup_etable(big_etable(dtable,n), slot, type, n);
    }
  }
}

int read_dispatch_table (VMState* vms, int format){
  int* trie_table = vms->trie_table[format];
  int table_offset = 0;
  while(1){
    int value = lookup_trie_table(vms, (TrieTable*)(trie_table + table_offset));
    if(value < 0) return -value - 1;
    table_offset = value;
  }
}
