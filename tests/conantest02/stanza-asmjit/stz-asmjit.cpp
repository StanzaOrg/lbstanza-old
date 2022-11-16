#include <asmjit/asmjit.h>
#include "stz-asmjit.h"
#include <stdio.h>

using namespace asmjit;
using namespace x86;

JitRuntime* jit_runtime_new(void) {
  JitAllocator::CreateParams params {};
  return new JitRuntime(&params);
}
void jit_runtime_delete(JitRuntime* rt) {
  
  delete rt;
}
void* jit_runtime_add(JitRuntime* rt, CodeHolder *c) {
  void* fn;
  rt->_add(&fn, c);
  return fn;
}
void jit_runtime_release(JitRuntime* rt, void *c) {
  rt->_release(c);
}
CodeHolder* code_holder_new(JitRuntime *rt) {
  auto c = new CodeHolder(NULL);
  c->init(rt->environment());
  return c;
}
int code_holder_label_offset(CodeHolder *c, Label *f) {
  return c->labelOffset(*f);
}
void code_holder_delete(CodeHolder *c) {
  delete c;
}
void code_holder_reset(CodeHolder *c) {
  c->reset(ResetPolicy::kHard);
}
int code_holder_size(CodeHolder *c) {
  return c->codeSize();
}
void code_holder_flatten(CodeHolder *c) {
  c->flatten();
}
Assembler* assembler_new(CodeHolder *c) {
  return new Assembler(c);
}
void assembler_delete(Assembler *a) {
  delete a;
}
Label* assembler_new_label(Assembler *a) {
  Label label = a->newLabel();
  return new Label(label);
}
void assembler_bind(Assembler *a, Label *label) {
  a->bind(*label);
}
void assembler_jmp_label(Assembler *a, Label *label) {
  a->jmp(*label);
}
void assembler_jmp_mem(Assembler *a, MemPtr *mem) {
  a->jmp(mem->value);
}
void assembler_jmp_reg(Assembler *a, Gp *reg) {
  a->jmp(*reg);
}
void assembler_je(Assembler *a, Label *label) {
  a->je(*label);
}
void assembler_jne(Assembler *a, Label *label) {
  a->jne(*label);
}
void assembler_js(Assembler *a, Label *label) {
  a->js(*label);
}
void assembler_jns(Assembler *a, Label *label) {
  a->jns(*label);
}
void assembler_jg(Assembler *a, Label *label) {
  a->jg(*label);
}
void assembler_jge(Assembler *a, Label *label) {
  a->jge(*label);
}
void assembler_jl(Assembler *a, Label *label) {
  a->jl(*label);
}
void assembler_jle(Assembler *a, Label *label) {
  a->jle(*label);
}
void assembler_ja(Assembler *a, Label *label) {
  a->ja(*label);
}
void assembler_jae(Assembler *a, Label *label) {
  a->jae(*label);
}
void assembler_jb(Assembler *a, Label *label) {
  a->jb(*label);
}
void assembler_jbe(Assembler *a, Label *label) {
  a->jbe(*label);
}
void assembler_and_reg(Assembler *a, const Gp *dst, const Gp *src) {
  a->and_(*dst, *src);
}
void assembler_and_int(Assembler *a, const Gp *dst, int src) {
  a->and_(*dst, src);
}
void assembler_or_reg(Assembler *a, const Gp *dst, const Gp *src) {
  a->or_(*dst, *src);
}
void assembler_xor_reg(Assembler *a, const Gp *dst, const Gp *src) {
  a->xor_(*dst, *src);
}
void assembler_not_reg(Assembler *a, const Gp *dst) {
  a->not_(*dst);
}
void assembler_neg_reg(Assembler *a, const Gp *dst) {
  a->neg(*dst);
}
void assembler_add_reg(Assembler *a, const Gp *dst, const Gp *src) {
  a->add(*dst, *src);
}
void assembler_imul_reg(Assembler *a, const Gp *dst, const Gp *src) {
  a->imul(*dst, *src);
}
void assembler_div_reg(Assembler *a, const Gp *divisor) {
  a->idiv(*divisor);
}
void assembler_mod_reg(Assembler *a, const Gp *divisor) {
  // TODO: FIX
  a->idiv(*divisor);
}
void assembler_add_int(Assembler *a, const Gp *dst, int src) {
  a->add(*dst, src);
}
void assembler_sub_int(Assembler *a, const Gp *dst, int src) {
  a->sub(*dst, src);
}
void assembler_sub_reg(Assembler *a, const Gp *dst, const Gp *src) {
  a->sub(*dst, *src);
}
void assembler_shl_int(Assembler *a, const Gp *dst, int src) {
  a->shl(*dst, src);
}
void assembler_shr_int(Assembler *a, const Gp *dst, int src) {
  a->shr(*dst, src);
}
void assembler_ashr_int(Assembler *a, const Gp *dst, int src) {
  a->sar(*dst, src);
}
void assembler_shl_reg(Assembler *a, const Gp *dst, const Gp *src) {
  a->shl(*dst, *src);
}
void assembler_shr_reg(Assembler *a, const Gp *dst, const Gp *src) {
  a->shr(*dst, *src);
}
void assembler_ashr_reg(Assembler *a, const Gp *dst, const Gp *src) {
  a->sar(*dst, *src);
}
void assembler_push(Assembler *a, Gp *reg) {
  a->push(*reg);
}
void assembler_pop(Assembler *a, Gp *reg) {
  a->pop(*reg);
}  
void assembler_call_label(Assembler *a, Label *f) {
  a->call(*f);
}
void assembler_call_reg(Assembler *a, Gp *reg) {
  a->call(*reg);
}
void assembler_ret(Assembler *a) {
  a->ret();
}
void assembler_movsx(Assembler *a, const Gp *dst, const Gp *src) {
  a->movsx(*dst, *src);
}
void assembler_movsxd(Assembler *a, const Gp *dst, const Gp *src) {
  a->movsxd(*dst, *src);
}

void assembler_mov_reg(Assembler *a, const Gp *dst, const Gp *src) {
  a->mov(*dst, *src);
}
void assembler_mov_xmm_reg(Assembler *a, const Xmm *dst, const Gp *src) {
  a->movq(*dst, *src);
}
void assembler_mov_reg_xmm(Assembler *a, const Gp *dst, const Xmm *src) {
  a->movq(*dst, *src);
}
void assembler_mov_int(Assembler *a, const Gp *reg, uint32_t value) {
  a->mov(*reg, uint32_t(value));
}
void assembler_mov_long(Assembler *a, const Gp *reg, uint64_t value) {
  a->mov(*reg, uint64_t(value));
}
void assembler_mov_label(Assembler *a, const Gp *reg, Label *value) {
  a->mov(*reg, uint64_t(value));
}
void assembler_mov_gp_ptr(Assembler *a, const Gp *reg, MemPtr* mem) {
  a->mov(*reg, mem->value);
}
void assembler_mov_ptr_gp(Assembler *a, const MemPtr* mem, Gp *reg) {
  a->mov(mem->value, *reg);
}
void assembler_lea_ptr(Assembler *a, const Gp *reg, MemPtr *mem) {
  a->lea(*reg, mem->value);
}
void assembler_cmp_reg(Assembler *a, const Gp *x, const Gp *y) {
  a->cmp(*x, *y);
}
void assembler_cmp_int(Assembler *a, const Gp *x, int y) {
  a->cmp(*x, y);
}
void assembler_set_e(Assembler *a, const Gp *x) {
  a->sete(*x);
}
void assembler_set_ne(Assembler *a, const Gp *x) {
  a->setne(*x);
}
void assembler_set_s(Assembler *a, const Gp *x) {
  a->sets(*x);
}
void assembler_set_ns(Assembler *a, const Gp *x) {
  a->setns(*x);
}
void assembler_set_g(Assembler *a, const Gp *x) {
  a->setg(*x);
}
void assembler_set_ge(Assembler *a, const Gp *x) {
  a->setge(*x);
}
void assembler_set_l(Assembler *a, const Gp *x) {
  a->setl(*x);
}
void assembler_set_le(Assembler *a, const Gp *x) {
  a->setle(*x);
}
void assembler_set_a(Assembler *a, const Gp *x) {
  a->seta(*x);
}
void assembler_set_ae(Assembler *a, const Gp *x) {
  a->setae(*x);
}
void assembler_set_b(Assembler *a, const Gp *x) {
  a->setb(*x);
}
void assembler_set_be(Assembler *a, const Gp *x) {
  a->setbe(*x);
}

void assembler_movss_xmm_xmm(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->movss(*dst, *src);
}
void assembler_movss_mem_xmm(Assembler *a, const Xmm *dst, MemPtr *src) {
  a->movss(*dst, src->value);
}
void assembler_movss_xmm_mem(Assembler *a, MemPtr *dst, const Xmm *src) {
  a->movss(dst->value, *src);
}
void assembler_movsd_xmm_xmm(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->movsd(*dst, *src);
}
void assembler_movsd_mem_xmm(Assembler *a, const Xmm *dst, MemPtr *src) {
  a->movsd(*dst, src->value);
}
void assembler_movsd_xmm_mem(Assembler *a, MemPtr *dst, const Xmm *src) {
  a->movsd(dst->value, *src);
}
void assembler_cvtss2sd(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->cvtss2sd(*dst, *src);
}
void assembler_cvtsd2ss(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->cvtsd2ss(*dst, *src);
}
void assembler_cvtsi2ss(Assembler *a, const Xmm *dst, const Gp *src) {
  a->cvtsi2ss(*dst, *src);
}
void assembler_cvtsi2sd(Assembler *a, const Xmm *dst, const Gp *src) {
  a->cvtsi2sd(*dst, *src);
}
void assembler_cvtsd2si(Assembler *a, const Gp *dst, const Xmm *src) {
  a->cvtsd2si(*dst, *src);
}
void assembler_cvtss2si(Assembler *a, const Gp *dst, const Xmm *src) {
  a->cvtss2si(*dst, *src);
}
void assembler_addss(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->addss(*dst, *src);
}
void assembler_addsd(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->addsd(*dst, *src);
}
void assembler_subss(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->subss(*dst, *src);
}
void assembler_subsd(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->subsd(*dst, *src);
}
void assembler_mulss(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->mulss(*dst, *src);
}
void assembler_mulsd(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->mulsd(*dst, *src);
}
void assembler_divss(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->divss(*dst, *src);
}
void assembler_divsd(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->divsd(*dst, *src);
}
void assembler_minss(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->minss(*dst, *src);
}
void assembler_minsd(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->minsd(*dst, *src);
}
void assembler_maxss(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->maxss(*dst, *src);
}
void assembler_maxsd(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->maxsd(*dst, *src);
}
void assembler_sqrtss(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->sqrtss(*dst, *src);
}
void assembler_sqrtsd(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->sqrtsd(*dst, *src);
}
void assembler_ucomiss(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->ucomiss(*dst, *src);
}
void assembler_ucomisd(Assembler *a, const Xmm *dst, const Xmm *src) {
  a->ucomisd(*dst, *src);
}

uint64_t func_call(Func f) {
  return f();
}
uint64_t func_call1(Func1 f, uint64_t arg) {
  return f(arg);
}
uint64_t func_call2(Func2 f, uint64_t a0, uint64_t a1) {
  return f(a0, a1);
}

const Gp* x86_al(void) {
  return &al;
}
const Gp* x86_bl(void) {
  return &bl;
}
const Gp* x86_cl(void) {
  return &cl;
}
const Gp* x86_dl(void) {
  return &dl;
}
const Gp* x86_sil(void) {
  return &sil;
}
const Gp* x86_dil(void) {
  return &dil;
}
const Gp* x86_spl(void) {
  return &spl;
}
const Gp* x86_bpl(void) {
  return &bpl;
}
const Gp* x86_r8b(void) {
  return &r8b;
}
const Gp* x86_r9b(void) {
  return &r9b;
}
const Gp* x86_r10b(void) {
  return &r10b;
}
const Gp* x86_r11b(void) {
  return &r11b;
}
const Gp* x86_r12b(void) {
  return &r12b;
}
const Gp* x86_r13b(void) {
  return &r13b;
}
const Gp* x86_r14b(void) {
  return &r14b;
}
const Gp* x86_r15b(void) {
  return &r15b;
}

const Gp* x86_eax(void) {
  return &eax;
}
const Gp* x86_ebx(void) {
  return &ebx;
}
const Gp* x86_ecx(void) {
  return &ecx;
}
const Gp* x86_edx(void) {
  return &edx;
}
const Gp* x86_esi(void) {
  return &esi;
}
const Gp* x86_edi(void) {
  return &edi;
}
const Gp* x86_esp(void) {
  return &esp;
}
const Gp* x86_ebp(void) {
  return &ebp;
}
const Gp* x86_r8d(void) {
  return &r8d;
}
const Gp* x86_r9d(void) {
  return &r9d;
}
const Gp* x86_r10d(void) {
  return &r10d;
}
const Gp* x86_r11d(void) {
  return &r11d;
}
const Gp* x86_r12d(void) {
  return &r12d;
}
const Gp* x86_r13d(void) {
  return &r13d;
}
const Gp* x86_r14d(void) {
  return &r14d;
}
const Gp* x86_r15d(void) {
  return &r15d;
}

const Gp* x86_rax(void) {
  return &rax;
}
const Gp* x86_rbx(void) {
  return &rbx;
}
const Gp* x86_rcx(void) {
  return &rcx;
}
const Gp* x86_rdx(void) {
  return &rdx;
}
const Gp* x86_rsi(void) {
  return &rsi;
}
const Gp* x86_rdi(void) {
  return &rdi;
}
const Gp* x86_rsp(void) {
  return &rsp;
}
const Gp* x86_rbp(void) {
  return &rbp;
}
const Gp* x86_r8(void) {
  return &r8;
}
const Gp* x86_r9(void) {
  return &r9;
}
const Gp* x86_r10(void) {
  return &r10;
}
const Gp* x86_r11(void) {
  return &r11;
}
const Gp* x86_r12(void) {
  return &r12;
}
const Gp* x86_r13(void) {
  return &r13;
}
const Gp* x86_r14(void) {
  return &r14;
}
const Gp* x86_r15(void) {
  return &r15;
}

const Xmm* x86_xmm0(void) {
  return &xmm0;
}
const Xmm* x86_xmm1(void) {
  return &xmm1;
}
const Xmm* x86_xmm2(void) {
  return &xmm2;
}
const Xmm* x86_xmm3(void) {
  return &xmm3;
}
const Xmm* x86_xmm4(void) {
  return &xmm4;
}
const Xmm* x86_xmm5(void) {
  return &xmm5;
}
const Xmm* x86_xmm6(void) {
  return &xmm6;
}
const Xmm* x86_xmm7(void) {
  return &xmm7;
}
const Xmm* x86_xmm8(void) {
  return &xmm8;
}
const Xmm* x86_xmm9(void) {
  return &xmm9;
}
const Xmm* x86_xmm10(void) {
  return &xmm10;
}
const Xmm* x86_xmm11(void) {
  return &xmm11;
}
const Xmm* x86_xmm12(void) {
  return &xmm12;
}
const Xmm* x86_xmm13(void) {
  return &xmm13;
}
const Xmm* x86_xmm14(void) {
  return &xmm14;
}
const Xmm* x86_xmm15(void) {
  return &xmm15;
}

const Rip* x86_rip(void) {
  return &rip;
}

const MemPtr* x86_ptr_gp_base_const_offset_size(Gp *base_ptr, int32_t offset, int32_t size) {
  auto base = *base_ptr;
  return new MemPtr(ptr(base, offset, size));
}
const MemPtr* x86_ptr_gp_base_index_const_shift_offset_size
    (Gp *base_ptr, Gp* index_ptr, int32_t shift, int32_t offset, int32_t size) {
  auto base = *base_ptr;
  auto index = *index_ptr;
  return new MemPtr(ptr(base, index, shift, offset, size));
}
const MemPtr* x86_ptr_label_base_index_const_shift_offset_size (Label *base_ptr, Gp* index_ptr, int32_t shift, int32_t offset, int32_t size) {
  auto base = *base_ptr;
  auto index = *index_ptr;
  return new MemPtr(ptr(base, index, shift, offset, size));
}
const MemPtr* x86_ptr_label_base_const_index_size (Label *base_ptr, int32_t index, int32_t size) {
  auto base = *base_ptr;
  return new MemPtr(ptr(base, index, size));
}

void assembler_embed_label(Assembler *a, Label *label) {
  a->embedLabel(*label);
}

void dump_registers (void) {
  uint64_t rax;
  asm("\t movq %%rax,%0" : "=r"(rax));  
  printf("RAX = %llx\n", rax);
  uint64_t rcx;
  asm("\t movq %%rcx,%0" : "=r"(rcx));  
  printf("RCX = %llx\n", rcx);
  uint64_t rdx;
  asm("\t movq %%rdx,%0" : "=r"(rdx));  
  printf("RDX = %llx\n", rdx);
  uint64_t rbx;
  asm("\t movq %%rbx,%0" : "=r"(rbx));  
  printf("RBX = %llx\n", rbx);
  uint64_t r8;
  asm("\t movq %%r8,%0" : "=r"(r8));  
  printf("R8 = %llx\n", r8);
  uint64_t r9;
  asm("\t movq %%r9,%0" : "=r"(r9));  
  printf("R9 = %llx\n", r9);
  uint64_t xmm0;
  asm("\t movd %%xmm0,%0" : "=r"(rax));  
  asm("\t movq %%rax,%0" : "=r"(xmm0));  
  printf("XMM0 = %llx\n", xmm0);
  uint64_t xmm1;
  asm("\t movd %%xmm1,%0" : "=r"(rax));  
  asm("\t movq %%rax,%0" : "=r"(xmm1));  
  printf("XMM1 = %llx\n", xmm1);
}
void dump_memory_64 (uint64_t* start, uint64_t n) {
  uint64_t i = 0;
  for (uint64_t* ptr = start; i < n; ptr += 1, i += 1) {
    printf("%p: %llx\n", ptr, *ptr);
  }
}
void dump_memory_32 (uint32_t* start, uint64_t n) {
  uint64_t i = 0;
  for (uint32_t* ptr = start; i < n; ptr += 1, i += 1) {
    printf("%p: %x\n", ptr, *ptr);
  }
}
void dump_memory_8 (uint8_t* start, uint64_t n) {
  uint64_t i = 0;
  for (uint8_t* ptr = start; i < n; ptr += 1, i += 1) {
    printf("%p: %x\n", ptr, *ptr);
  }
}
