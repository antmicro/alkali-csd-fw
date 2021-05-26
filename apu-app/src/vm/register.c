#include "vm.h"

void register_functions(struct ubpf_vm *vm)
{
	ubpf_register(vm, 1, "print", (void*)vm_print);
}
