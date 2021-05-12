#include "cmd.h"
#include "ramdisk.h"

typedef struct cmd_cdw12 {
	uint32_t nlb : 16;
	uint32_t rsvd0 : 4;
	uint32_t dtype : 4;
	uint32_t rsvd1 : 2;
	uint32_t prinfo : 4;
	uint32_t fua : 1;
	uint32_t lr : 1;
} cmd_cdw12_t;

typedef struct cmd_cdw13 {
	uint32_t freq : 4;
	uint32_t lat : 2;
	uint32_t seq : 1;
	uint32_t incomp : 1;
	uint32_t rsvd : 8;
	uint32_t dspec : 16;
} cmd_cdw13_t;

typedef struct cmd_cdw15 {
	uint32_t lbat : 16;
	uint32_t lbatm : 16;
} cmd_cdw15_t;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	uint32_t cdw10; // LBA 31:00
	uint32_t cdw11; // LBA 63:32
	cmd_cdw12_t cdw12;
	cmd_cdw13_t cdw13;
	uint32_t ilbrt;
	cmd_cdw15_t cdw15;
} cmd_sq_t;

void nvme_cmd_io_write(nvme_cmd_priv_t *priv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)priv->sq_buf;
	uint8_t *buf;

	uint32_t lba = cmd->cdw10;
	uint32_t nlb = cmd->cdw12.nlb + 1; // 0's based

#ifdef DEBUG
	printk("Ramdisk write: %d blocks from %d\n", nlb, lba);
#endif

	buf = ramdisk_write(lba, nlb);

	if(buf) {
		nvme_cmd_get_data(priv, (void*)buf, nlb*BLK_SIZE);
	} else {
		printk("Failed to get ramdisk address!\n");
		nvme_cmd_return(priv);
	}
}
