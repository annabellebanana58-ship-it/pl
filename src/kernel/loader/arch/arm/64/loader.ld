ENTRY(_start)
SECTIONS
{
    . = 0x80000;

    .text   : { KEEP(*(.text.boot)) *(.text*) }

    .rodata : { *(.rodata*) }

    .data   : { *(.data*) }

    .kernel_elf : ALIGN(8)
    {
        kernel_elf_start = .;
        KEEP(*(.kernel_elf))
        kernel_elf_end = .;
    }

    .bss (NOLOAD) : ALIGN(8)
    {
        __bss_start = .;
        *(.bss*) *(COMMON)
        . = ALIGN(8);
        __bss_end = .;
    }

    . = ALIGN(16);

    . += 0x10000;

    __stack_top = .;
}
