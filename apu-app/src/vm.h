#ifndef VM_H
#define VM_H

extern "C" {
#include "ubpf.h"
}

void register_functions(struct ubpf_vm *vm);

void vm_print(char *buf);

#endif
