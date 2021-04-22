/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <irq.h>

#include "nvme_tc.h"

enum State {IDLE = 0, RDY = 1};

typedef struct nvme_tc_priv {
	mem_addr_t base;
	enum State state;
} nvme_tc_priv_t;

nvme_tc_priv_t priv = {0};

void nvme_irq_handler(void *arg)
{
	printk("NVMe IRQ\n");

	while(sys_read32(priv.base + NVME_TC_IRQ_STA_REG)) {
		uint16_t reg = sys_read32(priv.base + NVME_TC_IRQ_DAT_REG) * 4;
		if(reg == NVME_TC_CC_REG) {
			uint32_t cc = sys_read32(priv.base + reg);
			if (cc & NVME_TC_CC_EN)
				sys_write32(1, priv.base + NVME_TC_CSTS_REG);
		}
		printk("Host write to reg 0x%04x\n", reg);
	}
}

void nvme_irq_init(void)
{
	printk("Configuring interrupts: %d\n", DT_INST_0_NVME_TC_IRQ_0);
	IRQ_CONNECT(DT_INST_0_NVME_TC_IRQ_0, DT_INST_0_NVME_TC_IRQ_0_PRIORITY, nvme_irq_handler, &priv, DT_INST_0_NVME_TC_IRQ_0_FLAGS);
	irq_enable(DT_INST_0_NVME_TC_IRQ_0);
	printk("Interrupts configured\n");
}

void nvme_tc_init(void)
{
	priv.base = (mem_addr_t)DT_INST_0_NVME_TC_BASE_ADDRESS;
	priv.state = IDLE;
}

void main(void)
{
	printk("NVMe Controller FW for %s\n", CONFIG_BOARD);

	nvme_tc_init();
	nvme_irq_init();

	while(1);
}
