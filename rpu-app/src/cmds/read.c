/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd.h"
#include "ramdisk.h"
#include "main.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(NVME_LOGGER_NAME, NVME_LOGGER_LEVEL);

typedef struct cmd_cdw12 {
	uint32_t nlb : 16;
	uint32_t rsvd : 10;
	uint32_t prinfo : 4;
	uint32_t fua : 1;
	uint32_t lr : 1;
} cmd_cdw12_t;

typedef struct cmd_cdw13 {
	uint32_t freq : 4;
	uint32_t lat : 2;
	uint32_t seq : 1;
	uint32_t incomp : 1;
	uint32_t rsvd : 24;
} cmd_cdw13_t;

typedef struct cmd_cdw15 {
	uint32_t alabat : 16;
	uint32_t elabatm : 16;
} cmd_cdw15_t;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	uint32_t cdw10; // LBA 31:00
	uint32_t cdw11; // LBA 63:32
	cmd_cdw12_t cdw12;
	cmd_cdw13_t cdw13;
	uint32_t eilbrt;
	cmd_cdw15_t cdw15;
} cmd_sq_t;

void nvme_cmd_io_read(nvme_cmd_priv_t *priv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)priv->sq_buf;
	uint8_t *buf;

	uint32_t lba = cmd->cdw10;
	uint32_t nlb = cmd->cdw12.nlb + 1; // 0's based

	LOG_DBG("lba[31:0] = 0x%X, lba[63:0] = 0x%X, nlb = %d\n", cmd->cdw10, cmd->cdw11, cmd->cdw12.nlb + 1);
	LOG_DBG("Ramdisk read: %d blocks from %d", nlb, lba);

	buf = ramdisk_read(lba, nlb);

	if (buf) {
		nvme_cmd_return_data(priv, (void*)buf, nlb*BLK_SIZE);
	} else {
		LOG_ERR("Failed to get ramdisk address!");
		nvme_cmd_return(priv);
	}
}
