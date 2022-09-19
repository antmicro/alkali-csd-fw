/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd.h"
#include "nvme_ident_fields.h"
#include "ramdisk.h"
#include "main.h"

#include <zephyr.h>
#include <sys/printk.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(NVME_LOGGER_NAME, NVME_LOGGER_LEVEL);

typedef struct cmd_cdw10 {
	uint32_t cns : 8;
	uint32_t rsvd : 8;
	uint32_t cntid : 16;
} cmd_cdw10_t;

typedef struct cmd_cdw11 {
	uint32_t nvmsetid : 16;
	uint32_t rsvd : 16;
} cmd_cdw11_t;

typedef struct cmd_cdw14 {
	uint32_t uuid_idx : 7;
	uint32_t rsvd : 25;
} cmd_cdw14_t;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	cmd_cdw10_t cdw10;
	cmd_cdw11_t cdw11;
	uint32_t rsvd[2];
	cmd_cdw14_t cdw14;
} cmd_sq_t;

#define NVME_CMD_IDENTIFY_RESP_SIZE	4096

#define CNS_IDENTIFY_NAMESPACE			0x00
#define CNS_IDENTIFY_CONTROLLER			0x01
#define CNS_IDENTIFY_NAMESPACE_LIST		0x02
#define CNS_IDENTIFY_NAMESPACE_IDENT_LIST	0x03

#define VID 0x1b96
#define SSVID VID
#define SN "0123456789"
#define MN "DEADBEEF"
#define FR "12.34"

#define OUI0 0x00
#define OUI1 0x90
#define OUI2 0xA9

#define VER 0x00010304

#define IO_CNTRL 1

#define SUBNQN "NVMe Open Source Controller"

static void fill_identify_struct(uint8_t *ptr)
{
	mem_addr_t buf = (mem_addr_t)ptr;
	memset(ptr, 0, NVME_CMD_IDENTIFY_RESP_SIZE);

	sys_write16(VID, buf + NVME_ID_FIELD_VID);
	sys_write16(SSVID, buf + NVME_ID_FIELD_SSVID);

	strncat(ptr + NVME_ID_FIELD_SN, SN, NVME_ID_FIELD_SN_SIZE-1);
	strncat(ptr + NVME_ID_FIELD_MN, MN, NVME_ID_FIELD_MN_SIZE-1);
	strncat(ptr + NVME_ID_FIELD_FR, FR, NVME_ID_FIELD_FR_SIZE-1);

	sys_write16(OUI0, buf + NVME_ID_FIELD_IEEE);
	sys_write16(OUI1, buf + NVME_ID_FIELD_IEEE + 2);
	sys_write16(OUI2, buf + NVME_ID_FIELD_IEEE + 4);

	sys_write32(VER, buf + NVME_ID_FIELD_VER);

	sys_write8(IO_CNTRL, buf + NVME_ID_FIELD_CNTRLTYPE);

	sys_write8(3, buf + NVME_ID_FIELD_ACL);
	sys_write8(3, buf + NVME_ID_FIELD_AERL);

	sys_write8(3, buf + NVME_ID_FIELD_FRMW);

	sys_write8(1, buf + NVME_ID_FIELD_AVSCC);

	sys_write16(0x0157, buf + NVME_ID_FIELD_WCTEMP);

	sys_write16(0x0300, buf + NVME_ID_FIELD_CCTEMP);

	sys_write8(128, buf + NVME_ID_FIELD_FWUG);

	sys_write8(0x66, buf + NVME_ID_FIELD_SQES);

	sys_write8(0x44, buf + NVME_ID_FIELD_CQES);

	sys_write32(1, buf + NVME_ID_FIELD_NN);

	sys_write8(1, buf + NVME_ID_FIELD_FNA);

	sys_write16(0xFFFF, buf + NVME_ID_FIELD_AWUN);

	sys_write16(0xFFFF, buf + NVME_ID_FIELD_AWUPF);

	sys_write8(1, buf + NVME_ID_FIELD_NVSCC);

	strncat(ptr + NVME_ID_FIELD_SUBNQN, SUBNQN, NVME_ID_FIELD_SUBNQN_SIZE-1);
}

static void identify_controller(nvme_cmd_priv_t *priv)
{
	static uint8_t resp_buf[NVME_CMD_IDENTIFY_RESP_SIZE];

	fill_identify_struct(resp_buf);

	nvme_cmd_return_data(priv, resp_buf, NVME_CMD_IDENTIFY_RESP_SIZE);
}

#define NSID 1

static void fill_identify_namespace_list(uint8_t *ptr)
{
	mem_addr_t buf = (mem_addr_t)ptr;
	memset(ptr, 0, NVME_CMD_IDENTIFY_RESP_SIZE);

	sys_write32(NSID, buf + NVME_ID_FIELD_VID);
}

static void identify_namespace_list(nvme_cmd_priv_t *priv)
{
	static uint8_t resp_buf[NVME_CMD_IDENTIFY_RESP_SIZE];

	fill_identify_namespace_list(resp_buf);

	nvme_cmd_return_data(priv, resp_buf, NVME_CMD_IDENTIFY_RESP_SIZE);
}

#define NVME_ID_FIELD_NIDT	0x0
#define NVME_ID_FIELD_NIDL	0x1
#define NVME_ID_FIELD_NID	0x4
#define NIDT_NUUID		0x3
#define NIDL_NUUID		0x10

static void identify_namespace_ident_list(nvme_cmd_priv_t *priv)
{
	static uint8_t resp_buf[NVME_CMD_IDENTIFY_RESP_SIZE];
	mem_addr_t buf = (mem_addr_t)resp_buf;
	memset(resp_buf, 0, NVME_CMD_IDENTIFY_RESP_SIZE);

	sys_write8(NIDT_NUUID, buf + NVME_ID_FIELD_NIDT);
	sys_write8(NIDL_NUUID, buf + NVME_ID_FIELD_NIDL);

	for(int i = 0; i < NIDL_NUUID; i++)
		sys_write8(i+1, buf + NVME_ID_FIELD_NID + i);

	nvme_cmd_return_data(priv, resp_buf, NVME_CMD_IDENTIFY_RESP_SIZE);
}

static void identify_namespace(nvme_cmd_priv_t *priv)
{
	static uint8_t resp_buf[NVME_CMD_IDENTIFY_RESP_SIZE];
	mem_addr_t buf = (mem_addr_t)resp_buf;
	memset(resp_buf, 0, NVME_CMD_IDENTIFY_RESP_SIZE);

	sys_write32(BLK_CNT, buf + 0);

	sys_write32(BLK_CNT, buf + 8);

	sys_write32((BLK_SHIFT << 16), buf + 128);

	nvme_cmd_return_data(priv, resp_buf, NVME_CMD_IDENTIFY_RESP_SIZE);
}

void nvme_cmd_adm_identify(nvme_cmd_priv_t *priv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)priv->sq_buf;

	switch(cmd->cdw10.cns) {
		case CNS_IDENTIFY_NAMESPACE:
			LOG_DBG("Handling CNS_IDENTIFY_NAMESPACE");
			identify_namespace(priv);
			break;
		case CNS_IDENTIFY_CONTROLLER:
			LOG_DBG("Handling CNS_IDENTIFY_CONTROLLER");
			identify_controller(priv);
			break;
		case CNS_IDENTIFY_NAMESPACE_LIST:
			LOG_DBG("Handling CNS_IDENTIFY_NAMESPACE_LIST");
			identify_namespace_list(priv);
			break;
		case CNS_IDENTIFY_NAMESPACE_IDENT_LIST:
			LOG_DBG("Handling CNS_IDENTIFY_NAMESPACE_IDENT_LIST");
			identify_namespace_ident_list(priv);
			break;
		default:
			LOG_WRN("Invalid Identify CNS value! (%d)", cmd->cdw10.cns);
			nvme_cmd_return(priv);
	}
}
