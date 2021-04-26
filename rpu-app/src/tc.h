#ifndef NVME_TC_H
#define NVME_TC_H

#include "nvme_reg_map.h"
#include "nvme_reg_fields.h"

#define DOORBELLS		5

#define DOORBELL_BASE		0x1000

#define DOORBELL_REG(n)		(DOORBELL_BASE+(n)*4)

#define DOORBELL_TAIL(n)	(DOORBELL_REG((n*2)))
#define DOORBELL_HEAD(n)	(DOORBELL_REG((n*2)+1))

#define NVME_TC_REG_ADM_TAIL	(DOORBELL_TAIL(0))
#define NVME_TC_REG_ADM_HEAD	(DOORBELL_HEAD(0))

#define NVME_TC_REG_IO_TAIL(n)	(DOORBELL_TAIL(n+1))
#define NVME_TC_REG_IO_HEAD(n)	(DOORBELL_HEAD(n+1))

#define NVME_TC_REG_IRQ_STA	(DOORBELL_TAIL(DOORBELLS))
#define NVME_TC_REG_IRQ_DAT	(DOORBELL_HEAD(DOORBELLS))

#define NVME_TC_GET_FIELD(reg,name)	(((reg) >> NVME_TC_REG_##name##_SHIFT) & NVME_TC_REG_##name##_MASK)

#define NVME_TC_SET_FIELD(reg, val, name)	(reg) |= (((val) & NVME_TC_REG_##name##_MASK) << NVME_TC_REG_##name##_SHIFT)

#define NVME_TC_CLR_FIELD(reg, name)	NVME_TC_SET_FIELD(reg, 0, name)

#define NVME_TC_SHUTDOWN_PROCESSING	0x1
#define NVME_TC_SHUTDOWN_COMPLETE	0x2

#define NVME_TC_ADM_SQ_ENTRY_SIZE	64
#define NVME_TC_ADM_SQ_SLAB_SIZE	256

void nvme_tc_irq_init(void);
void *nvme_tc_init(void *dma_priv);

#endif
