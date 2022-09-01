/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RPMSG_H
#define RPMSG_H

#include <stdint.h>

typedef struct payload {
	uint32_t id;
	uint32_t len;
	uint32_t priv;
	uint32_t buf;
	uint32_t buf_len;
	uint32_t data[];
} payload_t;

#define PAYLOAD_ADM_CMD		0x10
#define PAYLOAD_IO_CMD		0x11
#define PAYLOAD_ACK		0x20
#define PAYLOAD_ACK_DATA	0x21

#endif
