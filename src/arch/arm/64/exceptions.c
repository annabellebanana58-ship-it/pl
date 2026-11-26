#include "uart.h"
#include "kprint.h"
#include "exceptions.h"

void
exception_report(
    unsigned long esr,
    unsigned long elr,
    unsigned long spsr
)
{
    kprint("EXCEPTION!!!\n");

    kprint("Type: Synchronous exception\n");

    kprint("ESR_EL1:  %lx\n", esr);
    kprint("ESR_EL1:  %lx\n", elr);
    kprint("ESR_EL1:  %lx\n", spsr);

    kprint("System halted!!!\n");
}
