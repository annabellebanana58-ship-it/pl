/*
 * annabelle wrote this, i genuinely have no idea what it does
 * - aidan
 * 
 * Hi, I am Annabelle, this is,
 *
 * src/bootloader/arch/arm/boot.S
 * - annabelle
 */

.section ".text.boot"

.global _start
_start:
  mrs x1, mpidr_el1
  tst x1, #0xffffff
  b.eq primary_core
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
