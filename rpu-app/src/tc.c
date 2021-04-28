#include "tc.h"
#include "dma.h"
#include "cmd.h"

#include <sys/printk.h>

#include <string.h>
#include <math.h>

nvme_tc_priv_t p_tc = {0};

char __aligned(16) adm_sq_slab_buffer[NVME_TC_ADM_SQ_ENTRY_SIZE*NVME_TC_ADM_SQ_SLAB_SIZE];
char __aligned(16) adm_cq_slab_buffer[NVME_TC_ADM_CQ_ENTRY_SIZE*NVME_TC_ADM_CQ_SLAB_SIZE];

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

uint64_t nvme_tc_get_sq_addr(nvme_tc_priv_t *priv)
{
	uint64_t addr = priv->adm_sq_base + priv->adm_sq_head * NVME_TC_ADM_SQ_ENTRY_SIZE;
	uint32_t next_head = (priv->adm_sq_head + 1) % priv->adm_sq_size;

	priv->adm_sq_head = next_head;
	return addr;
}

void nvme_tc_cq_notify(nvme_tc_priv_t *priv, int queue_id)
{
	if(queue_id >= DOORBELLS) {
		printk("Invalid Queue ID!\n");
		return;
	}
	sys_write32(1<<queue_id,priv->base + NVME_TC_REG_IRQ_HOST);
}

uint64_t nvme_tc_get_cq_addr(nvme_tc_priv_t *priv)
{
	uint64_t addr;
	uint32_t next_tail = (priv->adm_cq_tail + 1) % priv->adm_cq_size;

	if(priv->adm_cq_head != next_tail) {
		if(priv->adm_cq_tail == 0) // We need to use flip the phase bit after each pass
			priv->adm_cq_phase = !priv->adm_cq_phase;

		addr = priv->adm_cq_base + priv->adm_cq_tail * NVME_TC_ADM_CQ_ENTRY_SIZE;
		priv->adm_cq_tail = next_tail;
	} else {
		addr = 0;
	}

	return addr;
}

static void nvme_tc_adm_tail_handler(nvme_tc_priv_t *priv)
{
	uint32_t tail = sys_read32(priv->base + NVME_TC_REG_ADM_TAIL);

	priv->adm_sq_tail = tail;

	while(priv->adm_sq_tail != priv->adm_sq_head) {
		void *dst;
		uint64_t host_addr = nvme_tc_get_sq_addr(priv);
		if(k_mem_slab_alloc(&priv->adm_sq_slab, &dst, K_NO_WAIT) == 0) {
			memset(dst, 0x5A, NVME_TC_ADM_SQ_ENTRY_SIZE);
			nvme_dma_xfer_host_to_mem(priv->dma_priv, host_addr, (uint32_t)dst, NVME_TC_ADM_SQ_ENTRY_SIZE, nvme_cmd_handle_adm, priv);
		} else {
			printk("Failed to allocate memory for Admin Queue entry");
		}
	}
}

static void nvme_tc_adm_head_handler(nvme_tc_priv_t *priv)
{
	uint32_t head = sys_read32(priv->base + NVME_TC_REG_ADM_HEAD);

	priv->adm_cq_head = head;
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
			case NVME_TC_REG_ADM_HEAD:
				nvme_tc_adm_head_handler(priv);
				break;
			default:
				printk("Register 0x%04x write not handled!\n", reg);
		}
	}
}

void nvme_tc_cq_update(void *arg, int queue_id, int cnt)
{
	nvme_tc_priv_t *priv = (nvme_tc_priv_t*)arg;	

	if(queue_id == 0) {
		priv->adm_cq_head = (priv->adm_cq_tail + 1) % priv->adm_cq_size;
		//sys_write32(priv->base + );
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
	k_mem_slab_init(&priv->adm_cq_slab, adm_cq_slab_buffer, NVME_TC_ADM_CQ_ENTRY_SIZE, NVME_TC_ADM_CQ_SLAB_SIZE);

	printk("Clearing registers\n");
	for(int i = 0; i < NVME_TC_REG_IRQ_STA; i+=4)
		sys_write32(0, priv->base + i);

	return (void*)priv;
}
