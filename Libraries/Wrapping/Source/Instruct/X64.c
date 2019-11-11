#include <Macro.h>
#include <Type.h>

void CallFunct(Void* UNUSED(callback), Void** UNUSED(params),
               Byte* UNUSED(types), UInt UNUSED(size), Void* UNUSED(result)) {
#if defined(__GNUC__) && defined(__clang__)
  __asm__("   pushq   %rdi                 ; ");
  __asm__("   pushq   %rsi                 ; ");
  __asm__("   pushq   %rdx                 ; ");
  __asm__("   pushq   %rcx                 ; ");
  __asm__("   pushq   %r8                  ; ");
  __asm__("   pushq   %r12                 ; ");
#endif

  /* @NOTE: store variable result to %r13, it will be used later */
  __asm__("   movq    %r8, %r13            ; ");
  __asm__("   xorq    %r12, %r12           ; ");

  /* @NOTE: the starting point where our altering code begins. This path is
   * shared with many functions */
  __asm__("begin:                            ");
#if defined(__GNUC__) && defined(__clang__)
  __asm__("   pushq   %rbx                 ; ");
  __asm__("   pushq   %r10                 ; ");
  __asm__("   pushq   %r11                 ; ");
  __asm__("   pushq   %r13                 ; ");
  __asm__("   pushq   %r14                 ; ");
  __asm__("   pushq   %r15                 ; ");
#endif

  __asm__("   xorq    %rax, %rax           ; ");
  __asm__("   xorq    %rbx, %rbx           ; ");
  __asm__("   movq    %rsi, %r10           ; ");
  __asm__("   movq    %rdi, %r11           ; ");
  __asm__("   movq    %rdx, %r14           ; ");
  __asm__("   movq    %rcx, %r15           ; ");
  __asm__("   cmpq    %rcx, %rax           ; ");
  __asm__("   je      finish               ; ");

  /* @NOTE: the loop0 is used to detect which path should be doing accoring the
   * type of our parameters */
  __asm__("loop0:                           ");
  __asm__("   cmpb    $1,(%r14,%rax)       ; ");
  __asm__("   jne     check_ltype          ; ");
  __asm__("check_ftype:                      ");
  __asm__("   addb    $1, %bh              ; ");
  __asm__("   cmpb    $8, %bh              ; ");
  __asm__("   jle     add_fvar             ; ");
  __asm__("   jmp     enqueue              ; ");
  __asm__("check_ltype:                      ");
  __asm__("   addb    $1, %bl              ; ");
  __asm__("   cmpb    $6, %bl              ; ");
  __asm__("   jle     add_lvar             ; ");
  __asm__("   jmp     enqueue              ; ");

  /* @NOTE: the loop1 is local looping where we only push parameter directly to
   *  our stack */
  __asm__("loop1:                            ");
  __asm__("   cmpq    %r15, %rax           ; ");
  __asm__("   je      finish               ; ");
  __asm__("enqueue:                          ");
  __asm__("   pushq   (%r10,%rax,8)        ; ");

  /* @NOTE: this tricky lines are used to detect which path should be doing
   * next, since we are mixing between floating point path, integer path and
   * enqueuing path. If resgister %bx is 0xffff, it should be handled locally
   * inside loop1. Otherwide, we still mantain our loop with loop0. */
  __asm__("increase:                         ");
  __asm__("   addq    $1, %rax             ; ");
  __asm__("   cmpq    %r15, %rax           ; ");
  __asm__("   jne     loop0                ; ");
  __asm__("   jmp     finish               ; ");

  /* @NOTE: we detect that this parameter is a floating point value and
   * we will choose the best way to store it here */
  __asm__("add_fvar:                         ");
  __asm__("size_fvar_1:                      ");
  __asm__("   cmpb    $1, %bh              ; ");
  __asm__("   jne     size_fvar_2          ; ");
  __asm__("   movsd   (%r10,%rax,8), %xmm0 ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_fvar_2:                      ");
  __asm__("   cmpb    $2, %bh              ; ");
  __asm__("   jne     size_fvar_3          ; ");
  __asm__("   movsd   (%r10,%rax,8), %xmm1 ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_fvar_3:                      ");
  __asm__("   cmpb    $3, %bh              ; ");
  __asm__("   jne     size_fvar_4          ; ");
  __asm__("   movsd   (%r10,%rax,8), %xmm2 ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_fvar_4:                      ");
  __asm__("   cmpb    $4, %bh              ; ");
  __asm__("   jne     size_fvar_5          ; ");
  __asm__("   movsd   (%r10,%rax,8), %xmm3 ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_fvar_5:                      ");
  __asm__("   cmpb    $5, %bh              ; ");
  __asm__("   jne     size_fvar_6          ; ");
  __asm__("   movsd   (%r10,%rax,8), %xmm4 ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_fvar_6:                      ");
  __asm__("   cmpb    $6, %bh              ; ");
  __asm__("   jne     size_fvar_7          ; ");
  __asm__("   movsd   (%r10,%rax,8), %xmm5 ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_fvar_7:                      ");
  __asm__("   cmpb    $7, %bh              ; ");
  __asm__("   jne     size_fvar_8          ; ");
  __asm__("   movsd   (%r10,%rax,8), %xmm6 ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_fvar_8:                      ");
  __asm__("   cmpb    $8, %bh              ; ");
  __asm__("   jne     size_fvar_9          ; ");
  __asm__("   movsd   (%r10,%rax,8), %xmm7 ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_fvar_9:                      ");
  __asm__("   movsd   (%r10,%rax,8), %xmm8 ; ");
  __asm__("   jmp     increase             ; ");

  /* @NOTE: we detect that this parameter shouldn't be floating point and we
   * should handle it in normal way as we have done with the old commits */
  __asm__("add_lvar:                         ");
  __asm__("size_lvar_1:                      ");
  __asm__("   cmpb    $1, %bl              ; ");
  __asm__("   jne     size_lvar_2          ; ");
  __asm__("   movq    (%r10,%rax,8), %rdi  ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_lvar_2:                      ");
  __asm__("   cmpb    $2, %bl              ; ");
  __asm__("   jne     size_lvar_3          ; ");
  __asm__("   movq    (%r10,%rax,8), %rsi  ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_lvar_3:                      ");
  __asm__("   cmpb    $3, %bl              ; ");
  __asm__("   jne     size_lvar_4          ; ");
  __asm__("   movq    (%r10,%rax,8), %rdx  ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_lvar_4:                      ");
  __asm__("   cmpb    $4, %bl              ; ");
  __asm__("   jne     size_lvar_5          ; ");
  __asm__("   movq    (%r10,%rax,8), %rcx  ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_lvar_5:                      ");
  __asm__("   cmpb    $5, %bl              ; ");
  __asm__("   jne     size_lvar_6          ; ");
  __asm__("   movq    (%r10,%rax,8), %r8   ; ");
  __asm__("   jmp     increase             ; ");
  __asm__("size_lvar_6:                      ");
  __asm__("   movq    (%r10,%rax,8), %r9   ; ");
  __asm__("   jmp     increase             ; ");

  /* @NOTE: ending part, we should call the callback here since every
   * parameters are fully loaded into approviated places */
  __asm__("finish:                           ");
  __asm__("   callq   %r11                 ; ");

  /* @NOTE: this path code is only reached if we ran the above code with
   * function CallFunct since we have to store the returning values from
   * the callback to %rax */
  __asm__("   cmpq    $0, %r13             ; ");
  __asm__("   je      revert0              ; ");
  __asm__("   movl    %eax,(%r13)          ; ");

  /* @NOTE: unlease the values are stucks inside our stack to prevent 
   * memory unalignement */
  __asm__("   xor    %eax, %eax            ; ");
  __asm__("revert0:                          ");
  __asm__("   cmpb    $8, %bh              ; ");
  __asm__("   jle     revert1              ; ");
  __asm__("   decb    %bh                  ; ");
  __asm__("   popq    %rax                 ; ");
  __asm__("   jmp     revert0              ; ");
  __asm__("revert1:                          ");
  __asm__("   cmpb    $6, %bl              ; ");
  __asm__("   jle     hijack               ; ");
  __asm__("   decb    %bl                  ; ");
  __asm__("   popq    %rax                 ; ");
  __asm__("   jmp     revert1              ; ");

  /* @NOTE: this tricky way would alter our control flow to the right
   * function after finishing our callback */
  __asm__("hijack:                           ");
#if defined(__GNUC__) && defined(__clang__)
  __asm__("   popq    %r15                 ; ");
  __asm__("   popq    %r14                 ; ");
  __asm__("   popq    %r13                 ; ");
  __asm__("   popq    %r11                 ; ");
  __asm__("   popq    %r10                 ; ");
  __asm__("   popq    %rbx                 ; ");
#endif

  __asm__("   cmpq    $0, %r12             ; ");
  __asm__("   je      release              ; ");
  __asm__("   jmp     %r12                 ; ");
  __asm__("release:                          ");

#if defined(__GNUC__) && defined(__clang__)
  /* @NOTE: restore the old value of rsi which contain. This line must run
   * only in function CallFunct */

  __asm__("   popq    %r12                 ; ");
  __asm__("   popq    %r8                  ; ");
  __asm__("   popq    %rcx                 ; ");
  __asm__("   popq    %rdx                 ; ");
  __asm__("   popq    %rsi                 ; ");
  __asm__("   popq    %rdi                 ; ");
#endif
}

void CallProc(Void* UNUSED(callback),  Void** UNUSED(params),
              Byte* UNUSED(types), UInt size) {
#if defined(__GNUC__) && defined(__clang__)
  __asm__("   pushq   %rdi                 ; ");
  __asm__("   pushq   %rsi                 ; ");
  __asm__("   pushq   %rdx                 ; ");
  __asm__("   pushq   %rcx                 ; ");
#endif

  /* @NOTE: prepare our parameters, the code altering only requires register
   * %r12 to be storing the ending address so it will jump back to our code
   * after finishing its control flow */
  __asm__("   pushq   %r12               ; ");
  __asm__("   leaq    end_callproc, %r12 ; ");
  __asm__("   jmp     begin              ; ");

  __asm__("end_callproc:                   ");
  __asm__("   popq    %r12               ; ");
#if defined(__GNUC__) && defined(__clang__)
  __asm__("   popq    %rcx                 ; ");
  __asm__("   popq    %rdx                 ; ");
  __asm__("   popq    %rsi                 ; ");
  __asm__("   popq    %rdi                 ; ");
#endif
}
