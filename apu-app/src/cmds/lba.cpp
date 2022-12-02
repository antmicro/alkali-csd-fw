/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd.h"
#include "lba.h"
#include "acc.h"

#include <cstdio>
#include <spdlog/spdlog.h>

void io_cmd_read_lba(payload_t *recv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)recv->data;
	const uint64_t lba = (((uint64_t)cmd->cdw13) << 32) | cmd->cdw12;
	const uint32_t len = cmd->cdw14.nlb;
	const uint64_t addr = (lba * RAMDISK_PAGE) + RAMDISK_BASE;
	const uint32_t id = cmd->cdw15;
#ifdef DEBUG
	spdlog::debug("Read LBA: {}, addr: {:02x}, len: {}, id: {}", lba, addr, len, id);

#ifdef BUFFER_TEST
	unsigned char *buf;
	int fd = -1;

	if(!mmap_buffer(addr, len*RAMDISK_PAGE, &fd, &buf)) {
		std::string logbuf = "";
		for(uint32_t i = 0; i < len*RAMDISK_PAGE; i++) {
			logbuf += fmt::format("{:02x} ", buf[i]);
			if((i % 16) == 15)
			{
				spdlog::debug(logbuf);
				logbuf = "";
			}
		}
		if (!logbuf.empty())
		{
			spdlog::debug(logbuf);
		}
		mmap_cleanup(len*RAMDISK_PAGE, fd, buf);
	}
#endif
#endif

	if(accelerators.size() > id)
		accelerators[id]->addRamdiskIn(addr, len*RAMDISK_PAGE);
	else
		spdlog::error("Invalid Accelerator ID! ({})", id);
}

void io_cmd_write_lba(payload_t *recv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)recv->data;
	const uint64_t lba = (((uint64_t)cmd->cdw13) << 32) | cmd->cdw12;
	const uint32_t len = cmd->cdw14.nlb;
	const uint64_t addr = (lba * RAMDISK_PAGE) + RAMDISK_BASE;
	const uint32_t id = cmd->cdw15;
#ifdef DEBUG
	spdlog::debug("Write LBA: {}, addr: {:lx}, len: {}, id: {}", lba, addr, len, id);

#ifdef BUFFER_TEST
	unsigned char *buf;
	int fd = -1;

	if(!mmap_buffer(addr, len*RAMDISK_PAGE, &fd, &buf)) {
	    	std::string logbuf = "";
		for(uint32_t i = 0; i < len*RAMDISK_PAGE; i++) {
			logbuf += format::fmt("{:02x} ", buf[i]);
			buf[i] = i;
			if((i % 16) == 15)
			{
				spdlog::debug(logbuf);
				logbuf = "";
			}
		}
		if (!logbuf.empty())
		{
			spdlog::debug(logbuf);
		}
		mmap_cleanup(len*RAMDISK_PAGE, fd, buf);
	}
#endif
#endif

	if(accelerators.size() > id)
		accelerators[id]->addRamdiskOut(addr, len*RAMDISK_PAGE);
	else
		spdlog::error("Invalid Accelerator ID! ({})", id);
}
