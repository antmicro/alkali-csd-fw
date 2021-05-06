#include "../cmd.h"

#include <zephyr.h>
#include <sys/printk.h>

typedef struct cmd_cdw10 {
	uint16_t qid : 16;
	uint16_t qsize : 16;
} cmd_cdw10_t;

typedef struct cmd_cdw11 {
	uint32_t pc : 1;
	uint32_t ien : 1;
	uint32_t rsvd : 14;
	uint32_t iv : 16;
} cmd_cdw11_t;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	cmd_cdw10_t cdw10;
	cmd_cdw11_t cdw11;
} cmd_sq_t;


void nvme_cmd_adm_create_sq(nvme_tc_priv_t *tc, void *buf, nvme_cq_entry_t *cq_buf)
{
	cmd_sq_t *cmd = (cmd_sq_t*)buf;	

	printk("QSIZE: %d, QID: %d, PC: %d, IEN: %d, IV: %d\n",
		cmd->cdw10.qsize,
		cmd->cdw10.qid,
		cmd->cdw11.pc,
		cmd->cdw11.ien,
		cmd->cdw11.iv
	);

	uint16_t qid = cmd->cdw10.qid;

	if(qid == 0 || qid > MAX_IO_QUEUES) {
		printk("Invalid Create SQ QID(%d)!\n", qid);
		cq_buf->sct = 1;
		cq_buf->sc = 1;
		return nvme_cmd_return(tc, &cmd->base, cq_buf);
	}

	qid -= 1;

	tc->io_sq_base[qid] = cmd->base.dptr.prp.prp1;

	tc->io_sq_head[qid] = 0;
	tc->io_sq_tail[qid] = 0;

	tc->io_sq_ien[qid] = cmd->cdw11.ien;
	tc->io_sq_pc[qid] = cmd->cdw11.pc;
	tc->io_sq_size[qid] = cmd->cdw10.qsize;
	tc->io_sq_iv[qid] = cmd->cdw11.iv;

	tc->io_cq_valid[qid] = true;

	nvme_cmd_return(tc, &cmd->base, cq_buf);
}

void nvme_cmd_adm_create_cq(nvme_tc_priv_t *tc, void *buf, nvme_cq_entry_t *cq_buf)
{
	cmd_sq_t *cmd = (cmd_sq_t*)buf;	

	printk("QSIZE: %d, QID: %d, PC: %d, IEN: %d, IV: %d\n",
		cmd->cdw10.qsize,
		cmd->cdw10.qid,
		cmd->cdw11.pc,
		cmd->cdw11.ien,
		cmd->cdw11.iv
	);

	uint16_t qid = cmd->cdw10.qid;

	if(qid == 0 || qid > MAX_IO_QUEUES) {
		printk("Invalid Create CQ QID(%d)!\n", qid);
		cq_buf->sct = 1;
		cq_buf->sc = 1;
		return nvme_cmd_return(tc, &cmd->base, cq_buf);
	}

	qid -= 1;

	tc->io_sq_base[qid] = cmd->base.dptr.prp.prp1;

	tc->io_cq_head[qid] = 0;
	tc->io_cq_tail[qid] = 0;
	tc->io_cq_phase[qid] = false;

	tc->io_cq_ien[qid] = cmd->cdw11.ien;
	tc->io_cq_pc[qid] = cmd->cdw11.pc;
	tc->io_cq_size[qid] = cmd->cdw10.qsize;
	tc->io_cq_iv[qid] = cmd->cdw11.iv;

	tc->io_cq_valid[qid] = true;

	nvme_cmd_return(tc, &cmd->base, cq_buf);
}
