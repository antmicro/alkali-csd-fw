#ifndef ACC_H
#define ACC_H

#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <map>

#include "vm.h"

#define ACC_IO_OP_RESET		0x00
#define ACC_IO_OP_START		0x01
#define ACC_IO_OP_STOP		0x02
#define ACC_IO_OP_SET_FW	0x03

enum class AccState {idle = 0, running, done, fail};

class Acc {
private:
	bool ramdisk_in;
	unsigned int ramdisk_in_base;
	unsigned int ramdisk_in_size;

	bool ramdisk_out;
	unsigned int ramdisk_out_base;
	unsigned int ramdisk_out_size;

	unsigned int id;

	void ramdiskToBuf(void);
	void bufToRamdisk(void);

	std::vector<unsigned char> firmware;
	std::vector<unsigned char> ibuf;
	std::vector<unsigned char> obuf;

	std::thread *th;

	std::atomic<AccState> state;

	void runBPF(void);
public:
	Acc(unsigned int id);
	unsigned int getId(void) { return id; }
	AccState getState(void) { return state; }
	void addRamdiskIn(unsigned int base, unsigned int size);
	void addRamdiskOut(unsigned int base, unsigned int size);
	void addFirmware(std::vector<unsigned char> &vec);
	void start(void);
	void stop(void);
};

extern std::vector<Acc*> accelerators;
extern std::map<unsigned int, std::vector<unsigned char>> fw_map;

#endif
