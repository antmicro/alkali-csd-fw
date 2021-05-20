/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dma.h"
#include <zephyr.h>
#include <sys/printk.h>
#include <string.h>

typedef struct nvme_dma_priv {
	mem_addr_t base;

	uint32_t tx_xfers;
	uint32_t rx_xfers;

	struct k_fifo tx_fifo;
	struct k_fifo rx_fifo;

	bool tx_dma_running;
	bool rx_dma_running;

	struct k_mem_slab desc_slab;
} nvme_dma_priv_t;

typedef struct nvme_dma_xfer_def {
	void *fifo_reserved;
	nvme_dma_xfer_cb *cb;
	void *cb_arg;
	uint64_t host_addr;
	uint32_t local_addr;
	uint32_t len;
	uint32_t tag;
} nvme_dma_xfer_def_t;

nvme_dma_priv_t p_dma = {0};

char __aligned(16) desc_slab_buffer[sizeof(nvme_dma_xfer_def_t)*NVME_DMA_SLAB_ENTRIES];

static void nvme_dma_setup_xfer(nvme_dma_priv_t *priv, nvme_dma_xfer_def_t *desc, uint32_t off)
{
#ifdef DEBUG
	printk("Configuring DMA transfer\n");
#endif

	__DMB();

	sys_write32(desc->host_addr & 0xffffffff, priv->base + off + NVME_DMA_REG_PCIE_ADDRL);
	sys_write32((desc->host_addr >> 32) & 0xffffffff, priv->base + off + NVME_DMA_REG_PCIE_ADDRH);

	// We use 32-bit AXI addresses, upper 32-bits are ignored
	sys_write32(desc->local_addr, priv->base + off + NVME_DMA_REG_AXI_ADDRL);
	sys_write32(0, priv->base + off + NVME_DMA_REG_AXI_ADDRH);

	sys_write32(desc->len, priv->base + off + NVME_DMA_REG_LEN);

	// Writing tag starts transfer
	sys_write32(desc->tag, priv->base + off + NVME_DMA_REG_TAG);
}

void nvme_dma_irq_handler(void *arg)
{
	nvme_dma_priv_t *priv = (nvme_dma_priv_t*)arg;
	unsigned long long lock;

	uint32_t read_status = sys_read32(priv->base + NVME_DMA_REG_STATUS + NVME_DMA_REG_READ_BASE);
	uint32_t write_status = sys_read32(priv->base + NVME_DMA_REG_STATUS + NVME_DMA_REG_WRITE_BASE);

	if(read_status & NVME_DMA_REG_STATUS_VALID) {
		nvme_dma_xfer_def_t *desc = k_fifo_get(&priv->rx_fifo, K_NO_WAIT);
		if(desc) {
			if(desc->cb)
				desc->cb(desc->cb_arg, (void*)desc->local_addr);
			k_mem_slab_free(&priv->desc_slab, (void**)&desc);
		} else {
			printk("Spurious DMA RX interrupt!\n");
		}

		lock = irq_lock();

		// Check if there is another transfer pending
		desc = k_fifo_peek_head(&priv->rx_fifo);
		if(desc)
			nvme_dma_setup_xfer(priv, desc, NVME_DMA_REG_READ_BASE);
		else
			priv->rx_dma_running = false;

		irq_unlock(lock);
	}

	if(write_status & NVME_DMA_REG_STATUS_VALID) {
		nvme_dma_xfer_def_t *desc = k_fifo_get(&priv->tx_fifo, K_NO_WAIT);
		if(desc) {
			if(desc->cb)
				desc->cb(desc->cb_arg, (void*)desc->local_addr);
			k_mem_slab_free(&priv->desc_slab, (void**)&desc);
		} else {
			printk("Spurious DMA TX interrupt!\n");
		}

		lock = irq_lock();

		// Check if there is another transfer pending
		desc = k_fifo_peek_head(&priv->tx_fifo);
		if(desc)
			nvme_dma_setup_xfer(priv, desc, NVME_DMA_REG_WRITE_BASE);
		else
			priv->tx_dma_running = false;

		irq_unlock(lock);
	}
}

void nvme_dma_irq_init(void)
{
	printk("Enabling DMA interrupts\n");
	IRQ_CONNECT(DT_INST_0_NVME_DMA_IRQ_0, DT_INST_0_NVME_DMA_IRQ_0_PRIORITY, nvme_dma_irq_handler, &p_dma, DT_INST_0_NVME_DMA_IRQ_0_FLAGS);
	irq_enable(DT_INST_0_NVME_DMA_IRQ_0);
	printk("DMA interrupts enabled\n");
}

void *nvme_dma_init(void)
{
	nvme_dma_priv_t *priv = &p_dma;

	priv->base = (mem_addr_t)DT_INST_0_NVME_DMA_BASE_ADDRESS;

	printk("Initializing private data\n");

	k_fifo_init(&priv->tx_fifo);
	k_fifo_init(&priv->rx_fifo);
	k_mem_slab_init(&priv->desc_slab, desc_slab_buffer, sizeof(nvme_dma_xfer_def_t), NVME_DMA_SLAB_ENTRIES);

	printk("Enabling DMA\n");
	sys_write32(1, priv->base + NVME_DMA_REG_EN);

	if(sys_read32(priv->base + NVME_DMA_REG_EN) == 1)
		printk("DMA enabled\n");
	else
		printk("Failed to enable DMA!\n");

	return (void*)priv;
}

static nvme_dma_xfer_def_t *nvme_dma_xfer_fill_desc(nvme_dma_priv_t *priv, uint64_t host, uint32_t local, uint32_t len, uint32_t tag, nvme_dma_xfer_cb *cb, void *cb_arg)
{
	nvme_dma_xfer_def_t *desc;

	if(k_mem_slab_alloc(&priv->desc_slab, (void**)&desc, K_NO_WAIT) == 0) {
		memset(desc, 0, sizeof(*desc));

		desc->host_addr = host;
		desc->local_addr = local;
		desc->len = len;
		desc->cb = cb;
		desc->cb_arg = cb_arg;
		desc->tag = tag;
	} else {
		printk("Failed to allocate DMA descriptor!\n");
		return NULL;
	}

	return desc;
}

int nvme_dma_xfer_host_to_mem(void* arg, uint64_t src, uint32_t dst, uint32_t len, nvme_dma_xfer_cb *cb, void *cb_arg)
{
	nvme_dma_priv_t *priv = (nvme_dma_priv_t*)arg;
	nvme_dma_xfer_def_t *desc = nvme_dma_xfer_fill_desc(priv, src, dst, len, 0xAA, cb, cb_arg);
	unsigned long long lock;

	if(desc) {
		lock = irq_lock();
		k_fifo_put(&priv->rx_fifo, desc);
		if(!priv->rx_dma_running) {
			nvme_dma_setup_xfer(priv, desc, NVME_DMA_REG_READ_BASE);
			priv->rx_dma_running = true;
		}
		irq_unlock(lock);
		return 0;
	} else {
		return -ENOMEM;
	}
}

int nvme_dma_xfer_mem_to_host(void* arg, uint32_t src, uint64_t dst, uint32_t len, nvme_dma_xfer_cb *cb, void *cb_arg)
{
	nvme_dma_priv_t *priv = (nvme_dma_priv_t*)arg;
	nvme_dma_xfer_def_t *desc = nvme_dma_xfer_fill_desc(priv, dst, src, len, 0x55, cb, cb_arg);
	unsigned long long lock;

	if(desc) {
		lock = irq_lock();
		k_fifo_put(&priv->tx_fifo, desc);
		if(!priv->tx_dma_running) {
			nvme_dma_setup_xfer(priv, desc, NVME_DMA_REG_WRITE_BASE);
			priv->tx_dma_running = true;
		}
		irq_unlock(lock);
		return 0;
	} else {
		return -ENOMEM;
	}
}
