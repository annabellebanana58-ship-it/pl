#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

void
exception_report(
    unsigned long esr,
    unsigned long elr,
    unsigned long spsr
);

#endif
