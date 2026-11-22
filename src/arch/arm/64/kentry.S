.section .text.boot

.global _kentry
_kentry:
    mov x19,     x0
    msr DAIFset, #0xf

    mrs  x1, mpidr_el1
    and  x1, x1, #0xff
    cbnz x1, halt_loop

    adrp x1, __bss_start
    add  x1, x1, :lo12:__bss_start
    adrp x2, __bss_end
    add  x2, x2, :lo12:__bss_end

bss_loop:
    cmp  x1,  x2
    b.ge bss_done
    str  xzr, [x1], #8
    b    bss_loop

bss_done:
    adrp x1,  __kernel_stack_top
    add  x1,  x1, :lo12:__kernel_stack_top
    mov  sp,  x1
    mov  x29, xzr

    mov x0, x19
    bl  kmain

halt_loop:
    wfe
    b   halt_loop
