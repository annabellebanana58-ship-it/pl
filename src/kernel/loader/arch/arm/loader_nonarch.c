#include <stdint.h>
#include <stddef.h>

#include "loader_arch.h"
#include "string.h"

/*
 * 	This is an architecture agnostic file, it provides functions that handle the logic irrelevant to architecture.
 *  It also functions as the main file for the loader.
 */

struct range {
	uint64_t start;
	uint64_t len;
};

/* https://devicetree-specification.readthedocs.io/en/v0.1/flattened-format.html */
#define FDT_MAGIC            0xd00dfeedU
#define FDT_HEADER_MIN_SIZE  40U

/* Big endian! */
static uint32_t
be32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
}

/* How many bytes does the device tree occupy?  */
static int
fdt_total_size(const void *dtb, uint64_t *size_out)
{
	const uint8_t *p = (const uint8_t *)dtb;

	if (be32(p) != FDT_MAGIC)
		return -1;

	uint64_t total = be32(p + 4);

	if (total < FDT_HEADER_MIN_SIZE)
		return -1;
	if (total > UINTPTR_MAX - (uintptr_t)dtb)
		return -1;

	*size_out = total;
	return 0;
}

static int
overlaps(uint64_t start, uint64_t len, const struct range *r)
{
	uint64_t end   = start + len;
	uint64_t r_end = r->start + r->len;

	if (len == 0 || r->len == 0)
		return 0;

	return start < r_end && r->start < end;
}

static int
is_segment_valid(uint64_t paddr,
				 uint64_t memsize,
				 uint64_t source_offset,
				 uint64_t filesize,
				 uint64_t total_blob_size,
				 const struct range *reserved,
				 size_t nreserved)
{
	/* Is the memory segment at least as large as the file data? */
	if (memsize < filesize) {
		return -1;
	}

	/* Does the file data lie within the embedded blob? */
	if (source_offset > total_blob_size ||
		filesize > total_blob_size - source_offset) {
		return -1;
		}

	/* Does the destination fit inside this target's address space? */
	if (paddr > UINTPTR_MAX ||
		memsize > UINTPTR_MAX - paddr) {
		return -1;
		}

			/* Does the destination avoid every reserved range (the loader itself, the DTB)? */
			for (size_t i = 0; i < nreserved; i++) {
				if (overlaps(paddr, memsize, &reserved[i])) {
					return -1;
				}
			}

			/* YAY! */
			return 0;
}

static void
load_segment_data(uint64_t paddr,
				  uint64_t source_ptr,
				  uint64_t memsize,
				  uint64_t filesize)
{
	memcpy((void *)(uintptr_t)paddr, (const void *)(uintptr_t)source_ptr, filesize);

	if  (memsize > filesize) {
		memset((void *)(uintptr_t)(paddr + filesize), 0, memsize - filesize);
	}
}

void
loader_main(void *dtb)
{
	extern uint8_t kernel_elf_start[];
	extern uint8_t kernel_elf_end[];
	extern uint8_t _start[];
	extern uint8_t __stack_top[];

	const uint64_t blob_size  = (uintptr_t)kernel_elf_end - (uintptr_t)kernel_elf_start;
	const uint64_t self_start = (uintptr_t)_start;
	const uint64_t self_len   = (uintptr_t)__stack_top - (uintptr_t)_start;

	/* Ranges that the kernel's segments must not overwrite. */
	struct range reserved[2];
	size_t nreserved = 0;

	reserved[nreserved].start = self_start;
	reserved[nreserved].len   = self_len;
	nreserved++;

	if (dtb != NULL) {
		uint64_t dtb_size;

		if (fdt_total_size(dtb, &dtb_size) != 0)
			arch_halt();

		reserved[nreserved].start = (uintptr_t)dtb;
		reserved[nreserved].len   = dtb_size;
		nreserved++;
	}

	if (blob_size < sizeof(elf_ehdr_t))
		arch_halt();

	const elf_ehdr_t *eh = (const elf_ehdr_t *)kernel_elf_start;

	if (eh->e_ident[EI_MAG0] != ELFMAG0 ||
		eh->e_ident[EI_MAG1] != ELFMAG1 ||
		eh->e_ident[EI_MAG2] != ELFMAG2 ||
		eh->e_ident[EI_MAG3] != ELFMAG3)
		arch_halt();

	if (eh->e_ident[EI_CLASS] != ELF_EXPECTED_CLASS ||
		eh->e_ident[EI_DATA]  != ELFDATA2LSB        ||
		eh->e_type            != ET_EXEC            ||
		eh->e_machine         != ELF_EXPECTED_MACHINE ||
		eh->e_phentsize       != sizeof(elf_phdr_t))
		arch_halt();

	/* That program header table oughta be inside that there blob I'll tell ya! */
	if ((uint64_t)eh->e_phoff > blob_size ||
		(uint64_t)eh->e_phnum * sizeof(elf_phdr_t) > blob_size - (uint64_t)eh->e_phoff)
		arch_halt();

	const elf_phdr_t *ph = (const elf_phdr_t *)(kernel_elf_start + eh->e_phoff);
	uint64_t entry = 0;
	uint64_t have_entry = 0;

	for (uint64_t i = 0; i < eh->e_phnum; i++) {
		if (ph[i].p_type != PT_LOAD)
			continue;

		if (is_segment_valid(ph[i].p_paddr,
			ph[i].p_memsz,
			ph[i].p_offset,
			ph[i].p_filesz,
			blob_size,
			reserved,
			nreserved) != 0) {
			arch_halt();
		}

		load_segment_data(ph[i].p_paddr,
						(uint64_t)(uintptr_t)(kernel_elf_start + ph[i].p_offset),
						ph[i].p_memsz,
						ph[i].p_filesz);

		arch_sync_icache_range((uintptr_t)ph[i].p_paddr,
							   (uintptr_t)(ph[i].p_paddr + ph[i].p_memsz));

		if (eh->e_entry >= ph[i].p_vaddr &&
			eh->e_entry - ph[i].p_vaddr < ph[i].p_memsz) {
			entry = (uint64_t)eh->e_entry - ph[i].p_vaddr + ph[i].p_paddr;
			have_entry = 1;
		}
	}

	if (!have_entry)
		arch_halt();

	arch_enter_kernel((uintptr_t)entry, dtb);
}
