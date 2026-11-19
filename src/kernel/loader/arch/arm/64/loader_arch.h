#ifndef LOADER_ARCH_H
#define LOADER_ARCH_H

#include <stdint.h>
#include <stddef.h>

#include "elf.h"

typedef Elf64_Ehdr elf_ehdr_t;
typedef Elf64_Phdr elf_phdr_t;
#define ELF_EXPECTED_CLASS    ELFCLASS64
#define ELF_EXPECTED_MACHINE  EM_AARCH64

/* This is a pre-C11 assert */
typedef char loader_assert_ehdr_size[(sizeof(elf_ehdr_t) == 64) ? 1 : -1];
typedef char loader_assert_phdr_size[(sizeof(elf_phdr_t) == 56) ? 1 : -1];

void arch_halt(void) __attribute__((noreturn));

void arch_sync_icache_range(uintptr_t start, uintptr_t end);

void arch_enter_kernel(uintptr_t entry, void *dtb) __attribute__((noreturn));

void loader_main(void *dtb);

#endif /* LOADER_ARCH_H */
