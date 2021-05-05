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

	sys_write8(100, buf + 3);
	sys_write8(5, buf + 4);
}

static void get_smart_log(nvme_tc_priv_t *tc, cmd_sq_t *cmd)
{
	static uint8_t resp_buf[SMART_RESP_SIZE];
	nvme_cq_entry_t *cq_buf;

	uint32_t len = (cmd->cdw11.numdu << 16) | cmd->cdw10.numdl;
	uint64_t off = ((uint64_t)cmd->cdw13) << 32 | cmd->cdw12;

	len = (len > SMART_RESP_SIZE) ? SMART_RESP_SIZE : len;

	if(off != 0)
		printk("Incorrect Get Log offset! (%llu)\n", off);

	if(k_mem_slab_alloc(&tc->adm_cq_slab, (void**)&cq_buf, K_NO_WAIT) == 0) {
		fill_smart_struct(resp_buf);
		fill_cq_resp(cq_buf, tc->adm_sq_head, cmd->base.cdw0.cid);

		nvme_cmd_return_data(tc, &cmd->base, resp_buf, len, cq_buf);
	}
}

void nvme_cmd_adm_get_log(nvme_tc_priv_t *tc, void *buf)
{
	cmd_sq_t *cmd = (cmd_sq_t*)buf;	

	switch(cmd->cdw10.lid) {
		case LID_SMART:
			get_smart_log(tc, cmd);
			break;
		default:
			printk("Invalid Get Log LID value! (%d)\n", cmd->cdw10.lid);
	}
}
