#ifndef NVME_DMA_H
#define NVME_DMA_H

#define NVME_DMA_REG_EN	0x000

#define NVME_DMA_REG_READ_BASE		0x100
#define NVME_DMA_REG_WRITE_BASE		0x200

#define NVME_DMA_REG_PCIE_ADDRL		0x00
#define NVME_DMA_REG_PCIE_ADDRH		0x04
#define NVME_DMA_REG_AXI_ADDRL		0x08
#define NVME_DMA_REG_AXI_ADDRH		0x0c
#define NVME_DMA_REG_LEN		0x10
#define NVME_DMA_REG_TAG		0x14
#define NVME_DMA_REG_STATUS		0x18
#define NVME_DMA_REG_STATUS_VALID	(1<<31)

#define NVME_DMA_REG_RQ_COUNT		0x400
#define NVME_DMA_REG_RC_COUNT		0x404
#define NVME_DMA_REG_CQ_COUNT		0x408
#define NVME_DMA_REG_CC_COUNT		0x40c

#define NVME_DMA_SLAB_ENTRIES		256

#include <stdint.h>

typedef void (nvme_dma_xfer_cb)(void *arg);

void nvme_dma_irq_init(void);
void *nvme_dma_init(void);
int nvme_dma_xfer_host_to_mem(void *arg, uint64_t src, uint32_t dst, uint32_t len, nvme_dma_xfer_cb *cb, void *cb_arg);
int nvme_dma_xfer_mem_to_host(void *arg, uint32_t src, uint64_t dst, uint32_t len, nvme_dma_xfer_cb *cb, void *cb_arg);

#endif
