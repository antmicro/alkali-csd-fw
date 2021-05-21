/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <irq.h>

#include "dma.h"
#include "tc.h"
#include "ramdisk.h"
#include "rpmsg.h"

#include "platform_info.h"

#include <string.h>

static int shutdown_req;

nvme_tc_priv_t *init(void)
{
	ramdisk_init();
	void *dma_priv = nvme_dma_init();
	nvme_tc_priv_t *tc = nvme_tc_init(dma_priv);

	nvme_dma_irq_init();
	nvme_tc_irq_init();

	rpmsg_init(tc);

	return tc;
}

void main(void)
{
	printk("NVMe Controller FW for %s\n", CONFIG_BOARD);

	nvme_tc_priv_t *tc = init();

	printk("Init complete\nController ready\n");

	while(1) {
		platform_poll(tc->platform);
		/* we got a shutdown request, exit */
		if (shutdown_req) {
			printk("Shutdown request!\n");
			break;
		}
	}
	rpmsg_destroy_ept(&tc->lept);

	for(;;);
}
