#include <stdint.h>
#include "string.h"
#include "elf.h"

/*
 * simple elf loader, the code is self explanatory
 * as a reminder for myself or anyone else, this needs a linker
 * script to actually work
 */

void
loader_main(void *dtb)
{
	(void)dtb;

	Elf64_Ehdr *eh = (Elf64_Ehdr *)kernel_elf_start;
	Elf64_Phdr *ph = (Elf64_Phdr *)((uint8_t *)kernel_elf_start + eh->e_phoff);

	if (eh->e_ident[EI_MAG0] != ELFMAG0 || eh->e_ident[EI_MAG1] != ELFMAG1 ||
		eh->e_ident[EI_MAG2] != ELFMAG2 || eh->e_ident[EI_MAG3] != ELFMAG3) {
		asm volatile("msr daifset, #0xf"); /* mask all interrupt sources */
		for (;;) {
			asm volatile("wfi");
		}
	}

	for (int i = 0; i < eh->e_phnum; i++, ph++) {
		if (ph->p_type != PT_LOAD)
			continue;

		memcpy((void *)ph->p_addr, kernel_elf_start + ph->p_offset, ph->p_filesz);
		memset((void *)(ph->p_addr + ph->p_filesz), 0, ph->p_memsz - ph->p_filesz);

		uint64_t start = ph->p_addr & ~63UL;
		uint64_t end   = ph->p_addr + ph->p_memsz;
		for (uint64_t addr = start; addr < end; addr += 64) {
			asm volatile("dc cvau, %0" :: "r"(addr));
		}
	}
	asm volatile("dsb ish");

	ph = (Elf64_Phdr *)((uint8_t *)kernel_elf_start + eh->e_phoff);
	for (int i = 0; i < eh->e_phnum; i++, ph++) {
		if (ph->p_type != PT_LOAD)
			continue;

		uint64_t start = ph->p_addr & ~63UL;
		uint64_t end   = ph->p_addr + ph->p_memsz;
		for (uint64_t addr = start; addr < end; addr += 64) {
			asm volatile("ic ivau, %0" :: "r"(addr));
		}
	}
	asm volatile("dsb ish");
	asm volatile("isb");

	void (*kernel_entry)(void *) = (void *)(uintptr_t)eh->e_entry;
	kernel_entry(dtb);
}
