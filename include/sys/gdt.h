#ifndef _GDT_H
#define _GDT_H

#include <sys/defs.h>

#define KERN_CS (1)
#define KERN_DS (2)

void reload_gdt();
void setup_tss();

#endif
