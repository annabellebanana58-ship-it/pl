#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define ELFMAG0         0x7f
#define ELFMAG1         'E'
#define ELFMAG2         'L'
#define ELFMAG3         'F'

#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8
#define EI_PAD 9
#define EI_NIDENT 16

#define ELFCLASS64   2
#define ELFDATA2LSB  1

#define ET_EXEC      2
#define EM_AARCH64   183

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_TLS 7
#define PT_LOOS 0x60000000
#define PT_HIOS 0x6fffffff
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

typedef struct {
	uint32_t      p_type;
	uint32_t      p_flags;
	uint64_t      p_offset;
	uint64_t      p_vaddr;
	uint64_t      p_paddr;
	uint64_t      p_filesz;
	uint64_t      p_memsz;
	uint64_t      p_align;
} __attribute__((packed)) Elf64_Phdr;

typedef struct {
	unsigned char e_ident[EI_NIDENT];
	uint16_t      e_type;
	uint16_t      e_machine;
	uint32_t      e_version;
	uint64_t      e_entry;
	uint64_t      e_phoff;
	uint64_t      e_shoff;
	uint32_t      e_flags;
	uint16_t      e_ehsize;
	uint16_t      e_phentsize;
	uint16_t      e_phnum;
	uint16_t      e_shentsize;
	uint16_t      e_shnum;
	uint16_t      e_shstrndx;
} __attribute__((packed)) Elf64_Ehdr;

#endif
