#include <stdint.h>
#include <stddef.h>

#include "string.h"
#include "loader_arch.h"

static inline void
dsb_ish(void)
{
    __asm__ volatile("dsb ish" ::: "memory");
}

static inline void
isb(void)
{
    __asm__ volatile("isb" ::: "memory");
}

void
arch_halt(void)
{
    __asm__ volatile("msr daifset, #0xf" ::: "memory");
    for (;;)
        __asm__ volatile("wfi");
}

void
arch_sync_icache_range(uintptr_t start, uintptr_t end)
{
    uint64_t ctr;
    __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));

    /* AArch64 specific cache line calculations */
    uint64_t dline = 4UL << ((ctr >> 16) & 0xf); /* DminLine */
    uint64_t iline = 4UL << (ctr & 0xf); /* IminLine */

    uint64_t d_start = start & ~(dline - 1);
    for (uint64_t a = d_start; a < end; a += dline) {
        __asm__ volatile("dc cvau, %0" :: "r"(a) : "memory");
    }
    dsb_ish();

    uint64_t i_start = start & ~(iline - 1);
    for (uint64_t a = i_start; a < end; a += iline) {
        __asm__ volatile("ic ivau, %0" :: "r"(a) : "memory");
    }
    dsb_ish();
    isb();
}

void
arch_enter_kernel(uintptr_t entry, void *dtb)
{
    void (*kentry)(void *) = (void (*)(void *))entry;
    kentry(dtb);

    arch_halt();
}
