#include "acc.h"
#include "cmd.h"

#include <cstdio>
#include <cstdlib>
#include <algorithm>

Acc::Acc(unsigned int id)
{
	this->id = id;

	state = AccState::idle;

	ramdisk_in = false;
	ramdisk_in_base = ramdisk_in_size = 0;

	ramdisk_out = false;
	ramdisk_out_base = ramdisk_out_size = 0;
}

void Acc::runBPF(void)
{
	char *errmsg;
	int ret;

	printf("[ACC#%d] Starting\n", id);

	state = AccState::running;

	struct ubpf_vm *vm = ubpf_create();

	if(!vm) {
		printf("[ACC#%d] Failed to create VM!\n", id);
		state = AccState::fail;
		return;
	}

	register_functions(vm);

	ret = ubpf_load_elf(vm, firmware.data(), firmware.size(), &errmsg);

	if(ret) {
		printf("[ACC#%d] Failed to load code: %s\n", id, errmsg);
		state = AccState::fail;
		free(errmsg);
		return;
	}

	if(ramdisk_in)
		ramdiskToBuf();

	if(ramdisk_out)
		obuf.resize(ramdisk_out_size);

	toggle_bounds_check(vm, false);

	ret = ubpf_exec(vm, ibuf.data(), ibuf.size(), obuf.data());

	if(ramdisk_out)
		bufToRamdisk();

	ubpf_destroy(vm);

	state = AccState::done;

	printf("[ACC#%d] Finished: %d\n", id, ret);

}

void Acc::ramdiskToBuf(void)
{
	unsigned char *buf;
	int fd;

	mmap_buffer(ramdisk_in_base, ramdisk_in_size, &fd, &buf);

	ibuf.assign(buf, buf+ramdisk_in_size);

	mmap_cleanup(ramdisk_in_size, fd, buf);
}

void Acc::bufToRamdisk(void)
{
	unsigned char *buf;
	int fd;

	mmap_buffer(ramdisk_out_base, ramdisk_out_size, &fd, &buf);

	std::copy(obuf.begin(), obuf.end(), buf);

	mmap_cleanup(ramdisk_out_size, fd, buf);
}

void Acc::addRamdiskIn(unsigned int base, unsigned int size)
{
	ramdisk_in = true;
	ramdisk_in_base = base;
	ramdisk_in_size = size;
}

void Acc::addRamdiskOut(unsigned int base, unsigned int size)
{
	ramdisk_out = true;
	ramdisk_out_base = base;
	ramdisk_out_size = size;
}

void Acc::addFirmware(std::vector<unsigned char> &vec)
{
	firmware = vec;
}

void Acc::start(void)
{
	th = new std::thread(&Acc::runBPF, this); 
}

void Acc::stop(void)
{

}
