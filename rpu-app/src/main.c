/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <irq.h>

#include "main.h"
#include "dma.h"
#include "tc.h"
#include "ramdisk.h"
#include "rpmsg.h"

#include "platform_info.h"

#include <string.h>

#include <logging/log_ctrl.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(NVME_LOGGER_NAME, NVME_LOGGER_LEVEL);
K_MEM_POOL_DEFINE(buffer_pool, PAGE_SIZE, NVME_BUFFER_SIZE, NVME_BUFFER_POOL_ENTRIES, PAGE_SIZE);

nvme_tc_priv_t *init(void)
{
	ramdisk_init();
	LOG_DBG("Ramdisk initialized");

	void *dma_priv = nvme_dma_init();
	LOG_DBG("DMA initialized");

	nvme_tc_priv_t *tc = nvme_tc_init(dma_priv);
	tc->buffer_pool = &buffer_pool;
	LOG_DBG("Target Controller (TC) initialized");

	nvme_dma_irq_init();
	LOG_DBG("DMA IRQ initialized");

	nvme_tc_irq_init();
	LOG_DBG("TC IRQs initialized");

	rpmsg_init(tc);
	LOG_DBG("Rpmsg initialized");

	return tc;
}

void main(void)
{
	log_init();
	LOG_INF("NVMe Controller FW for %s", CONFIG_BOARD);

	nvme_tc_priv_t *tc = init();

	LOG_INF("NVMe controller initialized");

	while(1) {
		platform_poll(tc->platform);
		k_sleep(100);
	}

	rpmsg_destroy_ept(&tc->lept);
}
