
  .global _c_trampoline
_c_trampoline:
  //function argument is in %rdi
  //buffer argument is in %rsi
  //ret argument is in %rdx      
  //Two tmps to use for duration of function: %r10 and %r11
  //Before setting registers, we can also use %rdx, %rcx, %r8, %r9 for tmps

  //Save buffer
  movq %rdx, -8(%rsp)        

  //Entry :
  //  Preserve SP (use rdx)
  //  Preserve F (use r11)
  //  Use TMP for buffer (use r10)
  movq %rsp, %rdx
  movq %rdi, %r11
  movq %rsi, %r10

  //Increment past saved buffer
  subq $8, %rsp        

  //Unload stack:
  //  Load stack count (use rcx)
  //  while stack count > 0 :
  //    copy one word from buffer to RSP (use r8 as tmp)
  //    increment buffer and RSP
  //    decrement stack count
  //  save previous %RSP on stack
  movq (%r10), %rcx
  addq $8, %r10
2:      
  cmpq $0, %rcx  
  je 1f
  subq $8, %rsp        
  movq (%r10), %r8
  movq %r8, (%rsp)
  addq $8, %r10
  subq $1, %rcx        
  jmp 2b
1:
  subq $8, %rsp
  andq $-16, %rsp //16-bit align
  movq %rdx, (%rsp)

  //State:
  // Preserved F (r11)
  // Buffer (r10)
  // Scratch: rdi, rsi, rdx, rcx, r8, r9

  //Unload fregs:
  // Load reg count (use rcx)
  // Lookup reg count in fregtable (use rdi for tmp)     
  movq (%r10), %rcx
  addq $8, %r10
  shlq $3, %rcx
  leaq fregtable(%rip), %rdi
  addq %rdi, %rcx
  movq (%rcx), %rcx
  jmp *%rcx
fregs16:        
  movq (%r10), %xmm15
  addq $8, %r10
fregs15:        
  movq (%r10), %xmm14
  addq $8, %r10
fregs14:        
  movq (%r10), %xmm13
  addq $8, %r10
fregs13:        
  movq (%r10), %xmm12
  addq $8, %r10
fregs12:        
  movq (%r10), %xmm11
  addq $8, %r10
fregs11:        
  movq (%r10), %xmm10
  addq $8, %r10
fregs10:        
  movq (%r10), %xmm9
  addq $8, %r10
fregs9: 
  movq (%r10), %xmm8
  addq $8, %r10
fregs8: 
  movq (%r10), %xmm7
  addq $8, %r10
fregs7: 
  movq (%r10), %xmm6
  addq $8, %r10
fregs6: 
  movq (%r10), %xmm5
  addq $8, %r10
fregs5: 
  movq (%r10), %xmm4
  addq $8, %r10
fregs4: 
  movq (%r10), %xmm3
  addq $8, %r10
fregs3: 
  movq (%r10), %xmm2
  addq $8, %r10
fregs2: 
  movq (%r10), %xmm1
  addq $8, %r10
fregs1: 
  movq (%r10), %xmm0
  addq $8, %r10
fregs0:

  //Unload intregs:
  // Load reg count (use rcx)
  // Lookup reg count in iregtable (use rdi for tmp)     
  movq (%r10), %rcx
  addq $8, %r10
  shlq $3, %rcx
  leaq iregtable(%rip), %rdi
  addq %rdi, %rcx
  movq (%rcx), %rcx
  jmp *%rcx
iregs7: 
  movq (%r10), %r9
  addq $8, %r10
iregs6: 
  movq (%r10), %r8
  addq $8, %r10
iregs5: 
  movq (%r10), %rcx
  addq $8, %r10
iregs4: 
  movq (%r10), %rdx
  addq $8, %r10
iregs3: 
  movq (%r10), %rsi
  addq $8, %r10
iregs2: 
  movq (%r10), %rdi
  addq $8, %r10
iregs1: 
  movq (%r10), %rax
  addq $8, %r10      
iregs0:

  // Call the function
  call *%r11
        
  // Return from function:
  // Restore RSP
  movq (%rsp), %rsp

  // Restore buffer (use rcx)
  movq -8(%rsp), %rcx

  // Write function returns out      
  movq %rax, 0(%rcx)
  movq %xmm0, 8(%rcx)      

  // Return      
  ret

.data
fregtable:
  .quad fregs0
  .quad fregs1
  .quad fregs2
  .quad fregs3
  .quad fregs4
  .quad fregs5
  .quad fregs6
  .quad fregs7
  .quad fregs8
  .quad fregs9
  .quad fregs10
  .quad fregs11
  .quad fregs12
  .quad fregs13
  .quad fregs14
  .quad fregs15
  .quad fregs16
iregtable:
  .quad iregs0
  .quad iregs1
  .quad iregs2
  .quad iregs3
  .quad iregs4
  .quad iregs5
  .quad iregs6
  .quad iregs7        
.text                
       
