/*
 * annabelle wrote this, i genuinely have no idea what it does
 * - aidan
 *
 * src/bootloader/arch/arm/boot.S
 *
 * Entry point for bootloader on arm64 for Raspberry Pi & QEMU raspi3b
 * Firmware jumps here with:
 *  core 0 executing, in EL2, MMU/caches off
 *  x0 = pointer to a device tree blob
 *  x1-x3 = reserved, must be zero
 * This file parks secondary cores, sets up a stack, zeroes .bss, and hands off to loader_main in loader.c
 */

.section ".text.boot" /* linker places this first */

.global _start
_start:
  /* all cores but 0 park */
  mrs x1, mpidr_el1 /* Multiprocessor affinity register */
  and x1, x1, #3 /* low 2 bits = core ID, which is 0-3 on a Pi 3 */
  cbz x1, primary_core /* if core ID == 0, fall through to boot */
park_loop:
  wfe /* sleep until core is signaled */
  b park_loop /* other cores never leave if we are in unicore */

primary_core:
  /* le stack pointer */
  ldr x1, =_start
  mov sp, x1

  /* .bss section zeroing
   * C thinks that glabol is at 0, but we need to do that ourselves
   * __bss_start and __bss_end will be defined in da linker */
  ldr x1, =__bss_start
  ldr x2, =__bss_end

zero_bss_loop:
  cmp x1, x2
  b.ge  call_loader
  str xzr, [x1], #8
  b zero_bss_loop

  /* jump into C */
call_loader:
  bl  loader_main /* this really shouldn't return, but the hang below makes sure that's 100% the case */
hang:
  wfe
  b hang
