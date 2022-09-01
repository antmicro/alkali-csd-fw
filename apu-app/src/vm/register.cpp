/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm.h"

void register_functions(struct ubpf_vm *vm)
{
	ubpf_register(vm, 1, "print", (void*)vm_print);
	ubpf_register(vm, 2, "tflite_apu", (void*)vm_tflite_apu);
	ubpf_register(vm, 3, "tflite_vta", (void*)vm_tflite_vta);
}
