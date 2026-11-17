#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "elf.h"

extern uint8_t kernel_elf_start[];
extern uint8_t kernel_elf_end[];
extern uint8_t _start[];
extern uint8_t __stack_top[];

static inline void
dsb_ish(void)
{
	asm volatile("dsb ish" ::: "memory");
}

static inline void
isb(void)
{
	asm volatile("isb" ::: "memory");
}

static void __attribute__((noreturn))
halt(void)
{
	asm volatile("msr daifset, #0xf" ::: "memory");
	for (;;) {
		asm volatile("wfi");
	}
}

static void
sync_icache_range(uint64_t start, uint64_t end)
{
	uint64_t ctr;
	asm volatile("mrs %0, ctr_el0" : "=r"(ctr));

	uint64_t dline = 4UL << ((ctr >> 16) & 0xf); /* DminLine */
	uint64_t iline = 4UL << (ctr & 0xf); /* IminLine */

	for (uint64_t a = start & ~(dline - 1); a < end; a += dline) {
		asm volatile("dc cvau, %0" :: "r"(a) : "memory");
	}
	dsb_ish();

	for (uint64_t a = start & ~(iline -1); a < end; a += iline) {
		asm volatile("ic ivau, %0" :: "r"(a) : "memory");
	}
	dsb_ish();
	isb();
}

static int
overlaps(uint64_t a, uint64_t alen, uint64_t b, uint64_t blen)
{
	return a < b + blen && b < a + alen;
}

void
loader_main(void *dtb)
{
	const uint64_t blob = (uintptr_t)kernel_elf_end - (uintptr_t)kernel_elf_start;
	const uint64_t self_start = (uintptr_t)_start;
	const uint64_t self_len = (uintptr_t)__stack_top - (uintptr_t)_start;

	if (blob < sizeof(Elf64_Ehdr))
		halt();
	
	const Elf64_Ehdr *eh = (const Elf64_Ehdr *)kernel_elf_start;
	
	if (eh->e_ident[EI_MAG0] != ELFMAG0 ||
	    eh->e_ident[EI_MAG1] != ELFMAG1 ||
	    eh->e_ident[EI_MAG2] != ELFMAG2 ||
	    eh->e_ident[EI_MAG3] != ELFMAG3  )
		halt();

	if (eh->e_ident[EI_CLASS] != ELFCLASS64 ||
            eh->e_ident[EI_DATA] != ELFDATA2LSB ||
       	    eh->e_type != ET_EXEC               ||
       	    eh->e_machine != EM_AARCH64         ||
	    eh->e_phentsize != sizeof(Elf64_Phdr))
               	halt();

	if (eh->e_phoff > blob || (uint64_t)eh->e_phnum * sizeof(Elf64_Phdr) > blob - eh->e_phoff)
		halt();

	const Elf64_Phdr *ph = (const Elf64_Phdr *)(kernel_elf_start + eh->e_phoff);

	uint64_t entry = 0;
	int have_entry = 0;

	for (int i = 0; i < eh->e_phnum; i++) {
		if (ph[i].p_type != PT_LOAD)
			continue;

		if (ph[i].p_memsz < ph[i].p_filesz)
			halt();
		if (ph[i].p_offset > blob || ph[i].p_filesz > blob - ph[i].p_offset)
			halt();
		if (ph[i].p_memsz > UINT64_MAX - ph[i].p_paddr)
			halt();
		if (overlaps(ph[i].p_paddr, ph[i].p_memsz, self_start, self_len))
			halt();

		memcpy((void *)ph[i].p_paddr, kernel_elf_start + ph[i].p_offset, ph[i].p_filesz);
		memset((void *)(ph[i].p_paddr + ph[i].p_filesz), 0, ph[i].p_memsz - ph[i].p_filesz);
		sync_icache_range(ph[i].p_paddr, ph[i].p_paddr + ph[i].p_memsz);

		if (eh->e_entry >= ph[i].p_vaddr && eh->e_entry - ph[i].p_vaddr < ph[i].p_memsz) {
			entry = eh->e_entry - ph[i].p_vaddr + ph[i].p_paddr;
			have_entry = 1;
		}
	}

	if (!have_entry)
		halt();

	void (*kernel_entry)(void *) = (void (*)(void *))(uintptr_t)entry;
	kernel_entry(dtb);

	halt();
}
