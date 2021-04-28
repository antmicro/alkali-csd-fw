#include "../cmd.h"

#include <zephyr.h>
#include <sys/printk.h>

#include <string.h>

typedef struct cmd_cdw10 {
	uint32_t cns : 8;
	uint32_t rsvd : 8;
	uint32_t cntid : 16;
} cmd_cdw10_t;

typedef struct cmd_cdw11 {
	uint32_t nvmsetid : 16;
	uint32_t rsvd : 16;
} cmd_cdw11_t;

typedef struct cmd_cdw14 {
	uint32_t uuid_idx : 7;
	uint32_t rsvd : 25;
} cmd_cdw14_t;

typedef struct cmd_cq {
	nvme_sq_entry_base_t base;
	cmd_cdw10_t cdw10;
	cmd_cdw11_t cdw11;
	uint32_t rsvd[2];
	cmd_cdw14_t cdw14;
} cmd_sq_t;

#define NVME_CMD_IDENTIFY_RESP_SIZE	4096

#define CNS_IDENTIFY_CONTROLLER		0x01

static void identify_controller(nvme_tc_priv_t *tc, cmd_sq_t *cmd)
{
	static uint8_t resp_buf[NVME_CMD_IDENTIFY_RESP_SIZE];
	nvme_cq_entry_t *cq_buf;

	if(k_mem_slab_alloc(&tc->adm_cq_slab, (void**)&cq_buf, K_NO_WAIT) == 0) {
		memset(resp_buf, 0, NVME_CMD_IDENTIFY_RESP_SIZE);
		memset((void*)cq_buf, 0, NVME_TC_ADM_CQ_ENTRY_SIZE);


		cq_buf->sq_head = tc->adm_sq_head;
		cq_buf->sq_id = 0;

		cq_buf->cid = cmd->base.cdw0.cid;

		cq_buf->p = tc->adm_cq_phase;
		
		cq_buf->sc = 0;
		cq_buf->sct = 0;

		cq_buf->crd = 0;
		cq_buf->m = 0;
		cq_buf->dnr = 0;

		nvme_cmd_return_data(tc, &cmd->base, resp_buf, NVME_CMD_IDENTIFY_RESP_SIZE, cq_buf);
	}
}

void nvme_cmd_adm_identify(nvme_tc_priv_t *tc, void *buf)
{
	cmd_sq_t *cmd = (cmd_sq_t*)buf;	

	switch(cmd->cdw10.cns) {
		case CNS_IDENTIFY_CONTROLLER:
			identify_controller(tc, cmd);
			break;
		default:
			printk("Invalid Identify CNS value! (%d)\n", cmd->cdw10.cns);
	}
}
