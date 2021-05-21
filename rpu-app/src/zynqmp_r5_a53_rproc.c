/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2017 Xilinx, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       zynqmp_r5_a53_rproc.c
 *
 * DESCRIPTION
 *
 *       This file define Xilinx ZynqMP R5 to A53 platform specific
 *       remoteproc implementation.
 *
 **************************************************************************/

#include <metal/atomic.h>
#include <metal/assert.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/utilities.h>
#include <openamp/rpmsg_virtio.h>
#include <drivers/ipm.h>
#include "platform_info.h"


static struct device *ipm_dev_tx;
static struct device *ipm_dev_rx;

int zynqmp_r5_a53_proc_irq_handler(int cpu_id, void *data)
{
	(void) cpu_id;
	struct remoteproc *rproc = data;
	struct remoteproc_priv *prproc;

	if (!rproc)
		return METAL_IRQ_NOT_HANDLED;

	prproc = rproc->priv;
	atomic_flag_clear(&prproc->ipi_nokick);

	return METAL_IRQ_HANDLED;
}

static struct remoteproc *
zynqmp_r5_a53_proc_init(struct remoteproc *rproc,
		struct remoteproc_ops *ops, void *arg)
{
	struct remoteproc_priv *prproc = arg;

	printk("remoteproc init\n");

	ipm_dev_tx = device_get_binding("MAILBOX_0");
	ipm_dev_rx = device_get_binding("MAILBOX_1");

	ipm_set_enabled(ipm_dev_rx, 1);

	if (!rproc || !prproc || !ops)
		return NULL;

	rproc->priv = prproc;

	atomic_store(&prproc->ipi_nokick, 1);
	rproc->ops = ops;

	return rproc;
}

static void zynqmp_r5_a53_proc_remove(struct remoteproc *rproc)
{
	ipm_set_enabled(ipm_dev_rx, 0);
}

static void *
zynqmp_r5_a53_proc_mmap(struct remoteproc *rproc, metal_phys_addr_t *pa,
			metal_phys_addr_t *da, size_t size,
			unsigned int attribute, struct metal_io_region **io)
{
	struct remoteproc_mem *mem;
	metal_phys_addr_t lpa, lda;
	struct metal_io_region *tmpio;

	lpa = *pa;
	lda = *da;

	if (lpa == METAL_BAD_PHYS && lda == METAL_BAD_PHYS)
		return NULL;
	if (lpa == METAL_BAD_PHYS)
		lpa = lda;
	if (lda == METAL_BAD_PHYS)
		lda = lpa;

	if (!attribute)
		attribute = NORM_SHARED_NCACHE | PRIV_RW_USER_RW;
	mem = metal_allocate_memory(sizeof(*mem));
	if (!mem)
		return NULL;
	tmpio = metal_allocate_memory(sizeof(*tmpio));
	if (!tmpio) {
		metal_free_memory(mem);
		return NULL;
	}
	remoteproc_init_mem(mem, NULL, lpa, lda, size, tmpio);
	/* va is the same as pa in this platform */
	metal_io_init(tmpio, (void *)lpa, &mem->pa, size,
			  sizeof(metal_phys_addr_t)<<3, attribute, NULL);
	remoteproc_add_mem(rproc, mem);
	*pa = lpa;
	*da = lda;
	if (io)
		*io = tmpio;
	return metal_io_phys_to_virt(tmpio, mem->pa);
}

static int zynqmp_r5_a53_proc_notify(struct remoteproc *rproc, uint32_t id)
{
	if (ipm_dev_tx) {
		ipm_send(ipm_dev_tx, 0, 0, NULL, 0);
	}
	return 0;
}

/* processor operations from r5 to a53. It defines
 * notification operation and remote processor managementi operations.
 */
struct remoteproc_ops zynqmp_r5_a53_proc_ops = {
	.init = zynqmp_r5_a53_proc_init,
	.remove = zynqmp_r5_a53_proc_remove,
	.mmap = zynqmp_r5_a53_proc_mmap,
	.notify = zynqmp_r5_a53_proc_notify,
	.start = NULL,
	.stop = NULL,
	.shutdown = NULL,
};
