/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RAMDISK_H
#define RAMDISK_H

#include <stdint.h>
#include <zephyr.h>

#define BLK_SHIFT	9
#define BLK_SIZE	(1<<BLK_SHIFT)
#define BLK_CNT		(DT_INST_1_MMIO_SRAM_SIZE/BLK_SIZE)

void ramdisk_init(void);

uint8_t *ramdisk_read(uint32_t lba, uint32_t nlb);
uint8_t *ramdisk_write(uint32_t lba, uint32_t nlb);

#endif
