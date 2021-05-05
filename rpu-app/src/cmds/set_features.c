#include "../cmd.h"

#include <zephyr.h>
#include <sys/printk.h>

typedef struct cmd_cdw10 {
	uint32_t fid : 8;
	uint32_t rsvd : 23;
	uint32_t sv : 1;
} cmd_cdw10_t;

typedef struct cmd_cdw14 {
	uint32_t uuid_idx : 7;
	uint32_t rsvd : 25;
} cmd_cdw14_t;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	cmd_cdw10_t cdw10;
	uint32_t cdw[3];
	cmd_cdw14_t cdw14;
} cmd_sq_t;

void nvme_cmd_adm_set_features(nvme_tc_priv_t *tc, void *buf)
{
	cmd_sq_t *cmd = (cmd_sq_t*)buf;	
	nvme_cq_entry_t *cq_buf;

	switch(cmd->cdw10.fid) {
		default:
			printk("Invalid Set Features FID value! (%d)\n", cmd->cdw10.fid);
	}

	if(k_mem_slab_alloc(&tc->adm_cq_slab, (void**)&cq_buf, K_NO_WAIT) == 0) {
		fill_cq_resp(cq_buf, tc->adm_sq_head, cmd->base.cdw0.cid);

		nvme_cmd_return(tc, &cmd->base, cq_buf);
	}
}
