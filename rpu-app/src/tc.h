/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NVME_TC_H
#define NVME_TC_H

#include "nvme_reg_map.h"
#include "nvme_reg_fields.h"
#include "dma.h"

#include <stdint.h>
#include <zephyr.h>

#include <openamp/open_amp.h>

#define IO_QUEUES		1

#define QUEUES	((IO_QUEUES)+1)

#define DOORBELLS		5

#define DOORBELL_BASE		0x1000

#define DOORBELL_REG(n)		(DOORBELL_BASE+(n)*4)

#define DOORBELL_TAIL(n)	(DOORBELL_REG((n*2)))
#define DOORBELL_HEAD(n)	(DOORBELL_REG((n*2)+1))

#define ADM_QUEUE_ID		0
#define ADM_QUEUE_IV		0

#define NVME_TC_REG_ADM_TAIL	(DOORBELL_TAIL(ADM_QUEUE_ID))
#define NVME_TC_REG_ADM_HEAD	(DOORBELL_HEAD(ADM_QUEUE_ID))

#define NVME_TC_REG_IO_TAIL(n)	(DOORBELL_TAIL(n+1))
#define NVME_TC_REG_IO_HEAD(n)	(DOORBELL_HEAD(n+1))

#define NVME_TC_REG_IRQ_STA	(DOORBELL_TAIL(DOORBELLS))
#define NVME_TC_REG_IRQ_DAT	(DOORBELL_HEAD(DOORBELLS))

#define NVME_TC_REG_IRQ_HOST	(NVME_TC_REG_IRQ_DAT+4)

#define NVME_TC_GET_FIELD(reg,name)	(((reg) >> NVME_TC_REG_##name##_SHIFT) & NVME_TC_REG_##name##_MASK)

#define NVME_TC_SET_FIELD(reg, val, name)	(reg) |= (((val) & NVME_TC_REG_##name##_MASK) << NVME_TC_REG_##name##_SHIFT)

#define NVME_TC_CLR_FIELD(reg, name)	NVME_TC_SET_FIELD(reg, 0, name)

#define NVME_TC_SHUTDOWN_PROCESSING	0x1
#define NVME_TC_SHUTDOWN_COMPLETE	0x2

#define NVME_TC_SQ_ENTRY_SIZE	64
#define NVME_TC_SQ_SLAB_SIZE	1024

#define NVME_TC_CQ_ENTRY_SIZE	16
#define NVME_TC_CQ_SLAB_SIZE	1024

#define NVME_CMD_SLAB_SIZE	1024

#define NVME_PRP_SLAB_SIZE	1024
#define NVME_PRP_LIST_SIZE	4096

void nvme_tc_irq_init(void);

typedef struct nvme_tc_priv {
	mem_addr_t base;
	bool enabled;

	void *dma_priv;

	/* RPU APU communication */

	struct rpmsg_device *rpdev;
	void *platform;
	struct rpmsg_endpoint lept;
	struct device *ipm_dev_tx;
	struct device *ipm_dev_rx;

	/* Queue parameters */

	int memory_page_size;
	int queues;

	struct k_mem_slab cmd_slab;
	struct k_mem_slab prp_slab;

	/* Submission Queues */

	bool sq_valid[QUEUES];
	uint16_t sq_size[QUEUES];
	uint64_t sq_base[QUEUES];
	uint16_t sq_tail[QUEUES];
	uint16_t sq_head[QUEUES];
	bool sq_pc[QUEUES];


	/* Completion Queues */

	bool cq_valid[QUEUES];
	uint16_t cq_size[QUEUES];
	uint64_t cq_base[QUEUES];
	uint16_t cq_tail[QUEUES];
	uint16_t cq_head[QUEUES];
	bool cq_pc[QUEUES];
	bool cq_phase[QUEUES];
	bool cq_ien[QUEUES];
	uint16_t cq_iv[QUEUES];
} nvme_tc_priv_t;

#define DIR_FROM_HOST 0
#define DIR_TO_HOST 1

typedef struct nvme_cmd_priv {
	int qid;
	nvme_tc_priv_t *tc;
	int dir;
	int prp_size;
	uint32_t xfer_base, xfer_size;
	uint32_t xfer_buf, xfer_len;
	nvme_dma_xfer_cb *xfer_cb;
	uint32_t sq_buf[NVME_TC_SQ_ENTRY_SIZE/4];
	uint32_t cq_buf[NVME_TC_CQ_ENTRY_SIZE/4];
} nvme_cmd_priv_t;

nvme_tc_priv_t *nvme_tc_init(void *dma_priv);

uint64_t nvme_tc_get_cq_addr(nvme_tc_priv_t *priv, const int qid);

void nvme_tc_cq_notify(nvme_tc_priv_t *priv, const int qid);

#endif
