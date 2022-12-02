/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd.h"
#include "nvme.h"
#include <cstdio>
#include <cassert>

#include "acc.h"
#include <spdlog/spdlog.h>
#define DEBUG

std::map<unsigned int, std::vector<unsigned char>> fw_map;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	uint32_t cdw10;
	uint32_t cdw11;
	uint32_t cdw12;
	uint32_t cdw13;
} cmd_sq_t;

void io_cmd_send_fw(payload_t *recv, unsigned char *buf)
{
	cmd_sq_t *cmd = (cmd_sq_t*)recv->data;
	const uint32_t len = cmd->cdw10*4;
	const uint32_t id = cmd->cdw13;
	assert(len == recv->buf_len); // For now we support only transfers that fit in a single buffer
	spdlog::debug("Received firmware (len: {}, id: {})", len, id);
	std::string logbuf = "";
	for(uint32_t i = 0; i < recv->buf_len; i++) {
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

	spdlog::debug("Map keys: ");
	logbuf = "";
	for(auto it = fw_map.begin(); it != fw_map.end(); it++) {
		logbuf += fmt::format("{} ", it->first);
	}

	spdlog::debug(logbuf);
	std::vector<unsigned char> fw(buf, buf+recv->buf_len);
	fw_map[id] = std::move(fw);
}
