#ifndef RPMSG_H
#define RPMSG_H

#include "tc.h"

int rpmsg_init(nvme_tc_priv_t *tc);

typedef struct nvme_rpmsg_payload {
	uint32_t id;
	uint32_t len;
	uint32_t priv;
	uint32_t data[];
} nvme_rpmsg_payload_t;

#define RPMSG_HANDLE_CUSTOM_ADM_COMMAND	0x10
#define RPMSG_HANDLE_CUSTOM_IO_COMMAND	0x11
#define RPMSG_SEND_COMPLETION 0x20

#endif
