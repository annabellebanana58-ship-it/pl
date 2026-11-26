.section .text.boot

.global _kentry
_kentry:
    mov x19,     x0
    msr DAIFset, #0xf

    mrs  x1, mpidr_el1
    and  x1, x1, #0xff
    cbnz x1, halt_loop

    mrs x0, CurrentEL
    cmp x0, #0x8
    b.eq from_el2

    cmp x0, #0x4
    b.eq from_el1

halt_loop:
    wfe
    b   halt_loop

from_el2:
    mov x0, #(1 << 31)
    msr hcr_el2, x0

    mov x0, #3
    msr cnthctl_el2, x0
    msr cntvoff_el2, xzr

    ldr x0, =__kernel_stack_top
    msr sp_el1, x0

    mov x0, #0x3c5
    msr spsr_el2, x0

    ldr x0, =el1_entry
    msr elr_el2, x0
    isb

    mov x0, #0x33ff
    msr cptr_el2, x0

    isb
    eret

from_el1:
    msr spsel, #1
    ldr x0, =__kernel_stack_top
    mov sp, x0

    b el1_entry

el1_entry:
    msr daifset, #0xf

    mov x0, #(3 << 20)
    msr cpacr_el1, x0
    isb

    ldr x0, =0x30d00800
    msr sctlr_el1, x0
    isb

    ldr x1, =__bss_start
    ldr x2, =__bss_end

bss_loop:
    cmp  x1,  x2
    b.hs bss_done
    str  xzr, [x1], #8
    b    bss_loop

bss_done:
    adrp x1,  __kernel_stack_top
    add  x1,  x1, :lo12:__kernel_stack_top
    mov  sp,  x1
    mov  x29, xzr

    bl install_vectors
    mov x0, x19
    bl  kmain

    b halt_loop

