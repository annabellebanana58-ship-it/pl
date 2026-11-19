ENTRY(_kstart)
SECTIONS
{
    . = 0x200000;
    .text : { *(.text*) }
}
