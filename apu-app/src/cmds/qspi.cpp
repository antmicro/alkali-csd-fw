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
#include <vector>
#include <algorithm>
#include <thread>

#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

#include <spdlog/spdlog.h>

const char mtd_path[] = "/dev/mtd0";

std::vector<unsigned char> fw_buffer;

std::thread *fw_thread = nullptr;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	uint32_t cdw10;
	uint32_t cdw11;
} cmd_sq_t;

#define DEBUG

static void flash_fw(const char *dev, std::vector<unsigned char> *buf)
{
	const int tgt_len = (int)buf->size();
	struct erase_info_user erase;
	struct mtd_info_user mtd;

	int fd = open(dev, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);

	if(fd == -1) {
		spdlog::error("Failed to open '{}'!", dev);
		return;
	}

	if(ioctl(fd, MEMGETINFO, &mtd) < 0) {
		spdlog::error("Failed to retrieve MTD device information!");
		return;
	}

	erase.start = 0;
	erase.length = mtd.size;

	if(ioctl(fd, MEMERASE, &erase) < 0) {
		spdlog::error("Failed to erase MTD device!");
		return;
	}

	spdlog::info("Writing %d bytes of firmware to {}", tgt_len, dev);

	int len = write(fd, buf->data(), tgt_len);

	close(fd);

	if(tgt_len != len) {
		spdlog::error("Failed to write {} bytes, ret: {}", tgt_len, len);
	} else {
		spdlog::info("{} bytes written", len);
	}

	delete buf;
}

void adm_cmd_fw_commit(payload_t *recv)
{
	if(fw_thread != nullptr) {
		fw_thread->join();
		delete fw_thread;
		fw_thread = nullptr;
	}

	std::vector<unsigned char> *buf = new std::vector<unsigned char>(fw_buffer);

	fw_thread = new std::thread(flash_fw, mtd_path, buf);
}

void adm_cmd_fw_download(payload_t *recv, unsigned char *buf)
{
	cmd_sq_t *cmd = (cmd_sq_t*)recv->data;
	const uint32_t ndw = cmd->cdw10;
	const uint32_t off = cmd->cdw11;

	if(fw_buffer.size() < ((off+ndw)*4)) {
		fw_buffer.resize((off+ndw)*4);
	}

	spdlog::debug("FW download len: {}, off: {:08x}", ndw*4, off*4);
	std::copy(buf, buf + ndw*4, fw_buffer.data()+off*4);
}
