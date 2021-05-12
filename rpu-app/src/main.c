/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <irq.h>

#include "dma.h"
#include "tc.h"
#include "ramdisk.h"

#include <string.h>

void init(void)
{
	ramdisk_init();
	void *dma_priv = nvme_dma_init();
	nvme_tc_init(dma_priv);

	nvme_dma_irq_init();
	nvme_tc_irq_init();
}

void main(void)
{
	printk("NVMe Controller FW for %s\n", CONFIG_BOARD);

	init();

	printk("Init complete\nController ready\n");

	while(1);
}
