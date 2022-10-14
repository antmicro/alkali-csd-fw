/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd.h"
#include "nvme.h"
#include "acc.h"

#include <cstdio>
#include <spdlog/spdlog.h>

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	uint32_t cdw10;
	uint32_t cdw11;
	uint32_t cdw12;
	uint32_t cdw13;
	uint32_t cdw14;
} cmd_sq_t;

void io_cmd_acc_ctl(payload_t *recv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)recv->data;
	const uint32_t id = cmd->cdw14;
	const uint32_t op = cmd->cdw13;
	const uint32_t fw_id = cmd->cdw14;
	spdlog::debug("IO CTL id: {}, op: {}", id, op);
	if(id >= accelerators.size()) {
		spdlog::error("Invalid accelerator ID! ({})", id);
		return;
	}

	Acc *a = accelerators[id];

	switch(op) {
		case ACC_IO_OP_RESET:
			accelerators[id] = new Acc(id);
			delete a;
			break;
		case ACC_IO_OP_START:
			a->start();
			break;
		case ACC_IO_OP_STOP:
			a->stop();
			break;
		case ACC_IO_OP_SET_FW:
			a->addFirmware(fw_map[fw_id]);
			break;
		default:
			spdlog::warn("Unsupported operation! ({})", op);
	}
}
