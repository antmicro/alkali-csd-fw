#ifndef LBA_H
#define LBA_H

#include "nvme.h"

#define RAMDISK_BASE	0x68000000
#define RAMDISK_PAGE	4096

typedef struct cmd_cdw14 {
	uint32_t nlb : 16;
	uint32_t rsvd : 16;
} cmd_cdw14_t;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	uint32_t cdw10;
	uint32_t cdw11;
	uint32_t cdw12; // LBA 31:00
	uint32_t cdw13; // LBA 63:32
	cmd_cdw14_t cdw14;
	uint32_t cdw15;
} cmd_sq_t;


#endif
