#ifndef NVME_TC_H
#define NVME_TC_H

#include "nvme_reg_map.h"
#include "nvme_reg_fields.h"

#include <stdint.h>
#include <zephyr.h>

#define DOORBELLS		5

#define DOORBELL_BASE		0x1000

#define DOORBELL_REG(n)		(DOORBELL_BASE+(n)*4)

#define DOORBELL_TAIL(n)	(DOORBELL_REG((n*2)))
#define DOORBELL_HEAD(n)	(DOORBELL_REG((n*2)+1))

#define ADM_QUEUE_ID		0

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

#define NVME_TC_ADM_SQ_ENTRY_SIZE	64
#define NVME_TC_ADM_SQ_SLAB_SIZE	256

#define NVME_TC_ADM_CQ_ENTRY_SIZE	16
#define NVME_TC_ADM_CQ_SLAB_SIZE	256

void nvme_tc_irq_init(void);
void *nvme_tc_init(void *dma_priv);

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
	uint32_t adm_sq_head;

	uint32_t adm_cq_tail;
	uint32_t adm_cq_head;
	uint32_t adm_cq_phase;

	struct k_mem_slab adm_sq_slab;
	struct k_mem_slab adm_cq_slab;

} nvme_tc_priv_t;

uint64_t nvme_tc_get_cq_addr(nvme_tc_priv_t *priv);

void nvme_tc_cq_notify(nvme_tc_priv_t *priv, int queue_id);

#endif
