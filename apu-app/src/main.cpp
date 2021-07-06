/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>
#include <cstdio>
#include <vector>

#include "nvme.h"
#include "rpmsg.h"
#include "cmd.h"
#include "acc.h"

std::vector<Acc*> accelerators;

int rpmsg_init(void);

static void setup_acc(void)
{
	Acc *a0 = new Acc(0);

	accelerators.push_back(a0);
}

static void init(void)
{
	setup_acc();
	setup_identify();
	setup_status();
}

int main(int argc, char *argv[])
{
	payload_t initial_msg = { .id = 1234, };
	char buf[NVME_TC_SQ_ENTRY_SIZE + sizeof(payload_t)];
	int fd = rpmsg_init();

	init();

	if(fd < 0)
		return fd;

	write(fd, &initial_msg, sizeof(initial_msg));

	for(;;) {
		int bytes = read(fd, buf, sizeof(buf));
		if(bytes > 0) {
			payload_t *recv = (payload_t*)buf;
			switch(recv->id) {
				case PAYLOAD_ADM_CMD:
					handle_adm_cmd(fd, recv);
					break;
				case PAYLOAD_IO_CMD:
					handle_io_cmd(fd, recv);
					break;
				default:
					printf("Unsupported command received! (id: %d, len: %d, priv: %08x)\n", recv->id, recv->len, recv->priv);
			}
		}
	}

	return 0;
}
