/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd.h"
#include "nvme.h"
#include <stdio.h>

bool global_enable = 0;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	uint32_t cdw10;
	uint32_t cdw11;
	uint32_t cdw12;	
} cmd_sq_t;

#define ACC_ENABLE	0x00
#define ACC_DISABLE	0x01

void adm_cmd_acc_ctl(payload_t *recv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)recv->data;

	switch(cmd->cdw12) {
		case ACC_ENABLE:
			global_enable = true;
#ifdef DEBUG
			printf("Enabling accelerator subsystem\n");
#endif
			break;
		case ACC_DISABLE:
			global_enable = false;
#ifdef DEBUG
			printf("Disabling accelerator subsystem\n");
#endif
			break;
		default:
			printf("Unknown operation ID (%d)\n", cmd->cdw12);
			break;
	}
}
