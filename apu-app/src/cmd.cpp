/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "nvme.h"
#include "cmd.h"

#include <spdlog/spdlog.h>

int mmap_buffer(uint32_t base, uint32_t len, int *fd, unsigned char **buf)
{
	*fd = open("/dev/mem", O_RDWR | O_SYNC);

	if(*fd == -1) {
		spdlog::error("Can't open /dev/mem ({})", errno);
		return errno;
	}

	*buf = (unsigned char*)mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, base);

	spdlog::debug("mapping memory (base: {0:x}, len: {})", base, len);

	if(*buf == MAP_FAILED) {
		close(*fd);
		spdlog::error("Can't map memory ({})", errno);
		return errno;
	}

	return 0;
}

void mmap_cleanup(uint32_t len, int fd, unsigned char *buf)
{
	munmap(buf, len);
	close(fd);
}

void send_ack(int fd, payload_t *data, uint32_t id)
{
	struct payload ack_msg = {
		id : id,
		priv : data->priv,
		buf : data->buf,
		buf_len : data->buf_len,
	};

	int bytes = write(fd, &ack_msg, sizeof(ack_msg));
	if(bytes != sizeof(ack_msg))
		spdlog::warn("Failed to send ACK: {}", bytes);
}

void handle_adm_cmd(int fd, payload_t *recv)
{
	nvme_sq_entry_base_t *cmd = (nvme_sq_entry_base_t*)recv->data;
	unsigned char *mmap_buf;
	int mmap_fd = -1;
	{
		uint32_t opc = cmd->cdw0.opc;
		spdlog::debug("ADM opc: {}", opc);
	}

	if(recv->buf_len > 0) {
		if(mmap_buffer(recv->buf, recv->buf_len, &mmap_fd, &mmap_buf)) {
			send_ack(fd, recv, PAYLOAD_ACK);
			return;
		}
	}

	switch(cmd->cdw0.opc) {
		case CMD_ADM_FW_COMMIT:
			adm_cmd_fw_commit(recv);
			send_ack(fd, recv, PAYLOAD_ACK);
			break;
		case CMD_ADM_FW_DOWNLOAD:
			adm_cmd_fw_download(recv, mmap_buf);
			send_ack(fd, recv, PAYLOAD_ACK);
			break;
		case CMD_ADM_IDENTIFY:
			adm_cmd_identify(recv, mmap_buf);
			send_ack(fd, recv, PAYLOAD_ACK_DATA);
			break;
		case CMD_ADM_CTL:
			adm_cmd_acc_ctl(recv);
			send_ack(fd, recv, PAYLOAD_ACK);
			break;
		case CMD_ADM_STATUS:
			adm_cmd_status(recv, mmap_buf);
			send_ack(fd, recv, PAYLOAD_ACK_DATA);
			break;
		default:
			send_ack(fd, recv, PAYLOAD_ACK);
			break;
	}

	if(mmap_fd != -1)
		mmap_cleanup(recv->buf_len, mmap_fd, mmap_buf);
}

void handle_io_cmd(int fd, payload_t *recv)
{
	nvme_sq_entry_base_t *cmd = (nvme_sq_entry_base_t*)recv->data;
	unsigned char *mmap_buf;
	int mmap_fd = -1;

	{
		uint32_t opc = cmd->cdw0.opc;
		spdlog::debug("IO opc: {}", opc);
	}

	if(recv->buf_len > 0) {
		if(mmap_buffer(recv->buf, recv->buf_len, &mmap_fd, &mmap_buf)) {
			send_ack(fd, recv, PAYLOAD_ACK);
			return;
		}
	}

	switch(cmd->cdw0.opc) {
		case CMD_IO_SEND_FW:
			io_cmd_send_fw(recv, mmap_buf);
			send_ack(fd, recv, PAYLOAD_ACK);
			break;
		case CMD_IO_READ_LBA:
			io_cmd_read_lba(recv);
			send_ack(fd, recv, PAYLOAD_ACK);
			break;
		case CMD_IO_WRITE_LBA:
			io_cmd_write_lba(recv);
			send_ack(fd, recv, PAYLOAD_ACK);
			break;
		case CMD_IO_SEND_DATA:
		case CMD_IO_READ_DATA:
		case CMD_IO_READ_FW:
		case CMD_IO_CTL:
			io_cmd_acc_ctl(recv);
			send_ack(fd, recv, PAYLOAD_ACK);
			break;
		default:
			send_ack(fd, recv, PAYLOAD_ACK);
			break;
	}

	if(mmap_fd != -1)
		mmap_cleanup(recv->buf_len, mmap_fd, mmap_buf);
}
