#include "../cmd.h"

#include <zephyr.h>
#include <sys/printk.h>

typedef struct cmd_cdw10 {
	uint32_t lid : 8;
	uint32_t lsp : 4;
	uint32_t rsvd : 3;
	uint32_t rae : 1;
	uint32_t numdl : 16;
} cmd_cdw10_t;

typedef struct cmd_cdw11 {
	uint32_t numdu : 16;
	uint32_t lsi : 16;
} cmd_cdw11_t;

typedef struct cmd_cdw14 {
	uint32_t uuid_idx : 7;
	uint32_t rsvd : 25;
} cmd_cdw14_t;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	cmd_cdw10_t cdw10;
	cmd_cdw11_t cdw11;
	uint32_t cdw12; // LPOL
	uint32_t cdw13; // LPOU
	cmd_cdw14_t cdw14;
} cmd_sq_t;

#define SMART_RESP_SIZE	512

#define LID_SMART	0x02

static void fill_smart_struct(uint8_t *ptr)
{
	mem_addr_t buf = (mem_addr_t)ptr;
	memset(ptr, 0, SMART_RESP_SIZE);

	sys_write16(298, buf + 1);
	sys_write8(100, buf + 3);
	sys_write8(5, buf + 4);

	sys_write32(2000, buf + 128);

	sys_write32(10, buf + 144);
	sys_write32(0, buf + 148);
	sys_write32(0, buf + 152);
	sys_write32(0, buf + 156);
}

static void get_smart_log(nvme_cmd_priv_t *priv, uint32_t len, uint64_t off)
{
	cmd_sq_t *cmd = (cmd_sq_t*)priv->sq_buf;
	static uint8_t resp_buf[SMART_RESP_SIZE];

	len = (len > SMART_RESP_SIZE) ? SMART_RESP_SIZE : len;

	if(off != 0)
		printk("Incorrect Get Log offset! (%llu)\n", off);

	fill_smart_struct(resp_buf);

	nvme_cmd_return_data(priv, resp_buf, len);
}

void nvme_cmd_adm_get_log(nvme_cmd_priv_t *priv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)priv->sq_buf;

	uint32_t len = ((cmd->cdw11.numdu << 16) | cmd->cdw10.numdl);
	uint64_t off = ((uint64_t)cmd->cdw13) << 32 | cmd->cdw12;

	len = (len+1) * 4; // len originally comes as number of dwords and is 0 based

	switch(cmd->cdw10.lid) {
		case LID_SMART:
			get_smart_log(priv, len, off);
			break;
		default:
			printk("Invalid Get Log LID value! (%d)\n", cmd->cdw10.lid);
	}
}
