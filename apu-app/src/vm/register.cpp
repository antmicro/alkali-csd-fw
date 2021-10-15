#include "vm.h"

void register_functions(struct ubpf_vm *vm)
{
	ubpf_register(vm, 1, "print", (void*)vm_print);
	ubpf_register(vm, 2, "tflite", (void*)vm_tflite);
	ubpf_register(vm, 3, "tflite_vta", (void*)vm_tflite_vta);
}
