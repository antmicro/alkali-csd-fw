/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>

#include <zephyr.h>
#include <sys/printk.h>
#include <irq.h>

#include "nvme.h"

typedef struct nvme_tc_priv {
	mem_addr_t base;
	bool enabled;

	int io_cq_entry_size;
	int io_sq_entry_size;
	int memory_page_size;

	int adm_cq_size;
	int adm_sq_size;

	uint64_t adm_sq_base;
	uint64_t adm_cq_base;

} nvme_tc_priv_t;

nvme_tc_priv_t p = {0};

void nvme_cc_handler(nvme_tc_priv_t *priv)
{
	uint32_t cc = sys_read32(priv->base + NVME_TC_REG_CC);
	uint32_t csts = sys_read32(priv->base + NVME_TC_REG_CSTS);

	priv->io_cq_entry_size = pow(2, NVME_GET_FIELD(cc, CC_IOCQES));
	priv->io_sq_entry_size = pow(2, NVME_GET_FIELD(cc, CC_IOSQES));

	if(NVME_GET_FIELD(cc, CC_SHN)) {
		printk("Shutdown notification detected\n");
		NVME_SET_FIELD(csts, NVME_SHUTDOWN_COMPLETE, CSTS_SHST);
	}

	if(NVME_GET_FIELD(cc, CC_AMS))
		printk("Unsupported arbitration method selected!\nWe only support Round Robin (000b)\n");

	priv->io_sq_entry_size = pow(2, NVME_GET_FIELD(cc, CC_MPS) + 12);

	if(NVME_GET_FIELD(cc, CC_CSS))
		printk("Unsupported command set selected!\nWe only support NVM command set (000b)\n");

	if(cc && NVME_TC_REG_CC_EN) {
		priv->enabled = true;
		printk("Controller enabled\n");
		csts |= NVME_TC_REG_CSTS_RDY;
	} else if (priv->enabled) {
		priv->enabled = false;
		printk("Controller reset requested\n");
		csts &= ~NVME_TC_REG_CSTS_RDY;
	}

	sys_write32(csts, priv->base + NVME_TC_REG_CSTS);
}

void nvme_aqa_handler(nvme_tc_priv_t *priv)
{
	uint32_t aqa = sys_read32(priv->base + NVME_TC_REG_AQA);

	priv->adm_cq_size = NVME_GET_FIELD(aqa, AQA_ACQS);
	priv->adm_sq_size = NVME_GET_FIELD(aqa, AQA_ASQS);
}

void nvme_asq_handler(nvme_tc_priv_t *priv)
{
	uint32_t asq0 = sys_read32(priv->base + NVME_TC_REG_ASQ_0);
	uint32_t asq1 = sys_read32(priv->base + NVME_TC_REG_ASQ_1);

	priv->adm_sq_base = ((uint64_t)asq0 << 32) | asq1;
}

void nvme_acq_handler(nvme_tc_priv_t *priv)
{
	uint32_t acq0 = sys_read32(priv->base + NVME_TC_REG_ACQ_0);
	uint32_t acq1 = sys_read32(priv->base + NVME_TC_REG_ACQ_1);

	priv->adm_cq_base = ((uint64_t)acq0 << 32) | acq1;
}

void nvme_irq_handler(void *arg)
{
	nvme_tc_priv_t *priv = (nvme_tc_priv_t*)arg;

	printk("NVMe IRQ\n");

	while(sys_read32(priv->base + NVME_TC_REG_IRQ_STA)) {
		uint16_t reg = sys_read32(priv->base + NVME_TC_REG_IRQ_DAT) * 4;
		printk("Host write to reg 0x%04x: 0x%08x\n", reg, sys_read32(priv->base + reg));
		switch(reg) {
			case NVME_TC_REG_CC:
				nvme_cc_handler(priv);
				break;
			case NVME_TC_REG_AQA:
				nvme_aqa_handler(priv);
				break;
			case NVME_TC_REG_ASQ_0:
				/* This will be handled in ASQ_1 handler */
				break;
			case NVME_TC_REG_ASQ_1:
				nvme_asq_handler(priv);
				break;
			case NVME_TC_REG_ACQ_0:
				/* This will be handled in ACQ_1 handler */
				break;
			case NVME_TC_REG_ACQ_1:
				nvme_acq_handler(priv);
				break;
			default:
				printk("Register 0x%04x write not handled!\n", reg);
		}
	}
}

void nvme_irq_init(void)
{
	printk("Configuring interrupts: %d\n", DT_INST_0_NVME_TC_IRQ_0);
	IRQ_CONNECT(DT_INST_0_NVME_TC_IRQ_0, DT_INST_0_NVME_TC_IRQ_0_PRIORITY, nvme_irq_handler, &p, DT_INST_0_NVME_TC_IRQ_0_FLAGS);
	irq_enable(DT_INST_0_NVME_TC_IRQ_0);
	printk("Interrupts configured\n");
}

void nvme_tc_init(nvme_tc_priv_t *priv)
{
	priv->base = (mem_addr_t)DT_INST_0_NVME_TC_BASE_ADDRESS;
	printk("Clearing registers\n");
	for(int i = 0; i < NVME_TC_REG_IRQ_STA; i+=4)
		sys_write32(0, priv->base + i);
}

void main(void)
{
	printk("NVMe Controller FW for %s\n", CONFIG_BOARD);

	nvme_tc_init(&p);
	nvme_irq_init();

	while(1);
}
