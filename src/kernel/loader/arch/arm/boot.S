/*
 * annabelle wrote this, i genuinely have no idea what it does
 * - aidan
 * 
 * Hi, I am Annabelle, this is,
 *
 * src/bootloader/arch/arm/boot.S
 *
 * Entry point for bootloader on arm64 for Raspberry Pi & QEMU raspi3b
 * Firmware jumps here with:
 *  core 0 executing, in EL2, MMU/caches off
 *  x0 = pointer to a device tree blob
 *  x1-x3 = reserved, must be zero
 * This file parks secondary cores, sets up a stack, zeroes .bss, and hands off to loader_main in loader.c
 * - annabelle
 */

.section ".text.boot"

.global _start
_start:
  mrs x1, mpidr_el1
  and x1, x1, #3
  cbz x1, primary_core
park_loop:
  wfe
  b park_loop

primary_core:
  ldr x1, =_start
  mov sp, x1

  ldr x1, =__bss_start
  ldr x2, =__bss_end

zero_bss_loop:
  cmp x1, x2
  b.ge  call_loader
  str xzr, [x1], #8
  b zero_bss_loop

  /* jump into C */
call_loader:
  bl  loader_main
hang:
  wfe
  b hang
