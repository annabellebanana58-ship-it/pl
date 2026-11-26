
        .section .text, "ax"

        .global install_vectors
        .type install_vectors, %function


install_vectors:
        ldr x0, =vectors
        msr vbar_el1, x0
        isb
        ret

        .size install_vectors, .-install_vectors

        .global sync_handler
        .type sync_handler, %function

sync_handler:
        sub sp, sp, #256

        stp x0,  x1,  [sp, #0]
        stp x2,  x3,  [sp, #16]
        stp x4,  x5,  [sp, #32]
        stp x6,  x7,  [sp, #48]
        stp x8,  x9,  [sp, #64]
        stp x10, x11, [sp, #80]
        stp x12, x13, [sp, #96]
        stp x14, x15, [sp, #112]
        stp x16, x17, [sp, #128]
        stp x18, x19, [sp, #144]
        stp x20, x21, [sp, #160]
        stp x22, x23, [sp, #176]
        stp x24, x25, [sp, #192]
        stp x26, x27, [sp, #208]
        stp x28, x29, [sp, #224]
        str x30, [sp, #240]

        /* Pass exception state to C function */
        mrs x0, esr_el1
        mrs x1, elr_el1
        mrs x2, spsr_el1

        bl exception_report

        /* Fatal exception */
1:
        wfe
        b 1b

        /* Temporary handlers: halt when reached */
irq_handler:
        b .

fiq_handler:
        b .

serror_handler:
        b .

        .section .text.vectors, "ax"
        .balign 2048

        .global vectors
        .type vectors, %object

        .macro vector label
        b \label
        .balign 128
        .endm

vectors:
        /* Current EL, SP0 */
        vector sync_handler
        vector irq_handler
        vector fiq_handler
        vector serror_handler

        /* Current EL, SPx */
        vector sync_handler
        vector irq_handler
        vector fiq_handler
        vector serror_handler

        /* Lower EL, AArch64 */
        vector sync_handler
        vector irq_handler
        vector fiq_handler
        vector serror_handler

        /* Lower EL, AArch32 */
        vector sync_handler
        vector irq_handler
        vector fiq_handler
        vector serror_handler

        .size vectors, .-vectors
