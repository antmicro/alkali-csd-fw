/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd.h"
#include "main.h"

#include <zephyr.h>
#include <sys/printk.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(NVME_LOGGER_NAME, NVME_LOGGER_LEVEL);

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

void nvme_cmd_adm_create_sq(nvme_cmd_priv_t *priv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)priv->sq_buf;
	nvme_tc_priv_t *tc = priv->tc;

	uint16_t qid = cmd->cdw10.qid;

	if(qid == 0 || qid > QUEUES) {
		LOG_ERR("Invalid Create SQ QID(%d)!", qid);
		//cq_buf->sct = 1;
		//cq_buf->sc = 1;
		return nvme_cmd_return(priv);
	}

	tc->sq_base[qid] = cmd->base.dptr.prp.prp1;

	tc->sq_head[qid] = 0;
	tc->sq_tail[qid] = 0;

	tc->sq_pc[qid] = cmd->cdw11.pc;
	tc->sq_size[qid] = cmd->cdw10.qsize + 1; // 0's based value

	tc->sq_valid[qid] = true;

	nvme_cmd_return(priv);
}

void nvme_cmd_adm_delete_sq(nvme_cmd_priv_t *priv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)priv->sq_buf;
	nvme_tc_priv_t *tc = priv->tc;

	uint16_t qid = cmd->cdw10.qid;

	if(qid == 0 || qid > QUEUES) {
		LOG_ERR("Invalid Create SQ QID(%d)!", qid);
		//cq_buf->sct = 1;
		//cq_buf->sc = 1;
		return nvme_cmd_return(priv);
	}

	tc->sq_valid[qid] = false;

	nvme_cmd_return(priv);
}

void nvme_cmd_adm_create_cq(nvme_cmd_priv_t *priv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)priv->sq_buf;
	nvme_tc_priv_t *tc = priv->tc;

	uint16_t qid = cmd->cdw10.qid;

	if(qid == 0 || qid > QUEUES) {
		LOG_ERR("Invalid Create CQ QID(%d)!", qid);
		//cq_buf->sct = 1;
		//cq_buf->sc = 1;
		return nvme_cmd_return(priv);
	}

	tc->cq_base[qid] = cmd->base.dptr.prp.prp1;

	tc->cq_head[qid] = 0;
	tc->cq_tail[qid] = 0;
	tc->cq_phase[qid] = false;

	tc->cq_ien[qid] = cmd->cdw11.ien;
	tc->cq_pc[qid] = cmd->cdw11.pc;
	tc->cq_size[qid] = cmd->cdw10.qsize + 1; // 0's based value
	tc->cq_iv[qid] = cmd->cdw11.iv;

	tc->cq_valid[qid] = true;

	nvme_cmd_return(priv);
}

void nvme_cmd_adm_delete_cq(nvme_cmd_priv_t *priv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)priv->sq_buf;
	nvme_tc_priv_t *tc = priv->tc;

	uint16_t qid = cmd->cdw10.qid;

	if(qid == 0 || qid > QUEUES) {
		LOG_ERR("Invalid Create CQ QID(%d)!", qid);
		//cq_buf->sct = 1;
		//cq_buf->sc = 1;
		return nvme_cmd_return(priv);
	}

	tc->cq_valid[qid] = false;

	nvme_cmd_return(priv);
}
