/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <irq.h>

void nvme_irq_handler(void *arg)
{
	printk("NVMe IRQ\n");
}

void nvme_irq_init()
{
	IRQ_CONNECT(DT_INST_0_NVME_TC_IRQ_0, DT_INST_0_NVME_TC_IRQ_0_PRIORITY, nvme_irq_handler, 0, DT_INST_0_NVME_TC_IRQ_0_FLAGS);
	irq_enable(DT_INST_0_NVME_TC_IRQ_0);
}

void main(void)
{
	printk("NVMe Controller FW for %s\n", CONFIG_BOARD);
	nvme_irq_init();
	while(1);
}
