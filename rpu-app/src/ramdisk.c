/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ramdisk.h"
#include "main.h"

#include <string.h>
#include <sys/printk.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(NVME_LOGGER_NAME, NVME_LOGGER_LEVEL);

#define RAMDISK_SIZE (BLK_SIZE*BLK_CNT)

static uint8_t *ramdisk_buffer;

void ramdisk_init()
{
	ramdisk_buffer = (uint8_t*)DT_INST_1_MMIO_SRAM_BASE_ADDRESS;

	LOG_INF("Creating ramdisk, nstart: %08x, blocks: %d, block size: %d", (uint32_t)ramdisk_buffer, BLK_CNT, BLK_SIZE);
	LOG_INF("Clearing first block");

	for(int i = 0; i < BLK_SIZE; i++)
		ramdisk_buffer[i] = i;
}

uint8_t *ramdisk_read(uint32_t lba, uint32_t nlb)
{
	return (lba <= BLK_CNT) ? &ramdisk_buffer[lba*BLK_SIZE] : NULL;
}

uint8_t *ramdisk_write(uint32_t lba, uint32_t nlb)
{
	if((lba <= BLK_CNT) && ((lba + nlb) <= BLK_CNT))
		return &ramdisk_buffer[lba*BLK_SIZE];
	else
		return NULL;
}
