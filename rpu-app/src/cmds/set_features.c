#include "cmd.h"

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

#define FID_NUMBER_OF_QUEUES	0x07

static void number_of_queues(nvme_cmd_priv_t *priv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)priv->sq_buf;
	nvme_cq_entry_t *cq = (nvme_cq_entry_t*)priv->cq_buf;

	uint16_t ncqr = (cmd->cdw[0] >> 16) & 0xFFFF;
	uint16_t nsqr = cmd->cdw[0] & 0xFFFF;

	printk("NCQR: %d\nNSQR: %d\n", ncqr, nsqr);

	// For now create only one IO queue

	priv->tc->queues = 1;

	cq->cdw0 = ((priv->tc->queues-1) << 16) | (priv->tc->queues-1);
}

void nvme_cmd_adm_set_features(nvme_cmd_priv_t *priv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)priv->sq_buf;

	switch(cmd->cdw10.fid) {
		case FID_NUMBER_OF_QUEUES:
			number_of_queues(priv);
			break;
		default:
			printk("Invalid Set Features FID value! (%d)\n", cmd->cdw10.fid);
	}

	nvme_cmd_return(priv);
}
