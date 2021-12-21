#ifndef VM_H
#define VM_H

extern "C" {
#include "ubpf.h"
}

void register_functions(struct ubpf_vm *vm);

void vm_print(char *buf);
void vm_tflite_apu(char *ibuf, char *obuf, int isize, int osize, int model_size);
void vm_tflite_vta(char *ibuf, char *obuf, int isize, int osize, int model_size);

#endif
