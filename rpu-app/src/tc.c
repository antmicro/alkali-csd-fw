#include "tc.h"
#include "dma.h"

#include <zephyr.h>
#include <sys/printk.h>

#include <string.h>
#include <stdint.h>
#include <math.h>

typedef struct nvme_tc_priv {
	mem_addr_t base;
	bool enabled;

	void *dma_priv;

	int io_cq_entry_size;
	int io_sq_entry_size;
	int memory_page_size;

	int adm_cq_size;
	int adm_sq_size;

	uint64_t adm_sq_base;
	uint64_t adm_cq_base;

	uint32_t adm_sq_tail;

	struct k_mem_slab adm_sq_slab;

} nvme_tc_priv_t;

nvme_tc_priv_t p_tc;

char __aligned(16) adm_sq_slab_buffer[NVME_TC_ADM_SQ_ENTRY_SIZE*NVME_TC_ADM_SQ_SLAB_SIZE];

static void nvme_tc_cc_handler(nvme_tc_priv_t *priv)
{
	uint32_t cc = sys_read32(priv->base + NVME_TC_REG_CC);
	uint32_t csts = sys_read32(priv->base + NVME_TC_REG_CSTS);

	priv->io_cq_entry_size = pow(2, NVME_TC_GET_FIELD(cc, CC_IOCQES));
	priv->io_sq_entry_size = pow(2, NVME_TC_GET_FIELD(cc, CC_IOSQES));

	if(NVME_TC_GET_FIELD(cc, CC_SHN)) {
		printk("Shutdown notification detected\n");
		NVME_TC_SET_FIELD(csts, NVME_TC_SHUTDOWN_COMPLETE, CSTS_SHST);
	}

	if(NVME_TC_GET_FIELD(cc, CC_AMS))
		printk("Unsupported arbitration method selected!\nWe only support Round Robin (000b)\n");

	priv->io_sq_entry_size = pow(2, NVME_TC_GET_FIELD(cc, CC_MPS) + 12);

	if(NVME_TC_GET_FIELD(cc, CC_CSS))
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

static void nvme_tc_aqa_handler(nvme_tc_priv_t *priv)
{
	uint32_t aqa = sys_read32(priv->base + NVME_TC_REG_AQA);

	priv->adm_cq_size = NVME_TC_GET_FIELD(aqa, AQA_ACQS);
	priv->adm_sq_size = NVME_TC_GET_FIELD(aqa, AQA_ASQS);
}

static void nvme_tc_asq_handler(nvme_tc_priv_t *priv)
{
	uint32_t asq0 = sys_read32(priv->base + NVME_TC_REG_ASQ_0);
	uint32_t asq1 = sys_read32(priv->base + NVME_TC_REG_ASQ_1);

	priv->adm_sq_base = ((uint64_t)asq1 << 32) | asq0;
}

static void nvme_tc_acq_handler(nvme_tc_priv_t *priv)
{
	uint32_t acq0 = sys_read32(priv->base + NVME_TC_REG_ACQ_0);
	uint32_t acq1 = sys_read32(priv->base + NVME_TC_REG_ACQ_1);

	priv->adm_cq_base = ((uint64_t)acq1 << 32) | acq0;
}

static void nvme_tc_adm_dma_handler(void *arg)
{
	nvme_tc_priv_t *priv = (nvme_tc_priv_t*)priv;

	printk("ADM Queue DMA handler\n");
}

static void nvme_tc_adm_tail_handler(nvme_tc_priv_t *priv)
{
	uint32_t tail = sys_read32(priv->base + NVME_TC_REG_ADM_TAIL);

	int diff = tail - priv->adm_sq_tail;

	if(diff > 0) {
		for(int i = 0; i < diff; i++) {
			void *dst;
			uint64_t host_addr = priv->adm_sq_base + ((i + priv->adm_sq_tail) % priv->adm_sq_size) * NVME_TC_ADM_SQ_ENTRY_SIZE;
			if(k_mem_slab_alloc(&priv->adm_sq_slab, &dst, K_NO_WAIT) == 0) {
				memset(dst, 0x5A, NVME_TC_ADM_SQ_ENTRY_SIZE);
				nvme_dma_xfer_host_to_mem(priv->dma_priv, host_addr, (uint32_t)dst, NVME_TC_ADM_SQ_ENTRY_SIZE, nvme_tc_adm_dma_handler, priv);
			} else {
				printk("Failed to allocate memory for Admin Queue entry");
			}
		}

		priv->adm_sq_tail = (priv->adm_sq_tail + diff) % priv->adm_sq_size;
	}
}

static void nvme_tc_irq_handler(void *arg)
{
	nvme_tc_priv_t *priv = (nvme_tc_priv_t*)arg;

	while(sys_read32(priv->base + NVME_TC_REG_IRQ_STA)) {
		uint16_t reg = sys_read32(priv->base + NVME_TC_REG_IRQ_DAT) * 4;
#ifdef DEBUG
		printk("Host write to reg 0x%04x: 0x%08x\n", reg, sys_read32(priv->base + reg));
#endif
		switch(reg) {
			case NVME_TC_REG_CC:
				nvme_tc_cc_handler(priv);
				break;
			case NVME_TC_REG_AQA:
				nvme_tc_aqa_handler(priv);
				break;
			case NVME_TC_REG_ASQ_0:
				/* This will be handled in ASQ_1 handler */
				break;
			case NVME_TC_REG_ASQ_1:
				nvme_tc_asq_handler(priv);
				break;
			case NVME_TC_REG_ACQ_0:
				/* This will be handled in ACQ_1 handler */
				break;
			case NVME_TC_REG_ACQ_1:
				nvme_tc_acq_handler(priv);
				break;
			case NVME_TC_REG_ADM_TAIL:
				nvme_tc_adm_tail_handler(priv);
				break;
			default:
				printk("Register 0x%04x write not handled!\n", reg);
		}
	}
}

void nvme_tc_irq_init(void)
{
	printk("Enabling TC interrupts\n");
	IRQ_CONNECT(DT_INST_0_NVME_TC_IRQ_0, DT_INST_0_NVME_TC_IRQ_0_PRIORITY, nvme_tc_irq_handler, &p_tc, DT_INST_0_NVME_TC_IRQ_0_FLAGS);
	irq_enable(DT_INST_0_NVME_TC_IRQ_0);
	printk("TC interrupts enabled\n");
}

void *nvme_tc_init(void *dma_priv)
{
	nvme_tc_priv_t *priv = &p_tc;

	priv->base = (mem_addr_t)DT_INST_0_NVME_TC_BASE_ADDRESS;

	priv->dma_priv = dma_priv;

	k_mem_slab_init(&priv->adm_sq_slab, adm_sq_slab_buffer, NVME_TC_ADM_SQ_ENTRY_SIZE, NVME_TC_ADM_SQ_SLAB_SIZE);

	printk("Clearing registers\n");
	for(int i = 0; i < NVME_TC_REG_IRQ_STA; i+=4)
		sys_write32(0, priv->base + i);

	return (void*)priv;
}
