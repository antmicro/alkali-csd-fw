#include "cmd.h"
#include "nvme.h"
#include "acc.h"

#include <cstdio>
#include <cstring>

typedef struct cmd_cdw12 {
	uint32_t id : 16;
	uint32_t rsvd : 15;
	uint32_t rae : 1;
} cmd_cdw12_t;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	uint32_t cdw10;
	uint32_t cdw11;
	cmd_cdw12_t cdw12;
} cmd_sq_t;

typedef struct stat_head {
	uint32_t len;
	uint32_t id;
	uint32_t rsvd[6];
} stat_head_t;

stat_head_t *heads;

void setup_status(void) {
	auto size = accelerators.size();

	heads = new stat_head_t[size];
}

static void calculate_status(unsigned int id)
{
	Acc *a = accelerators[id];

	stat_head_t &head = heads[id];

	head.len = 8;
	head.id = static_cast<uint32_t>(a->getState());
}

void adm_cmd_status(payload_t *recv, unsigned char *buf)
{
	cmd_sq_t *cmd = (cmd_sq_t*)recv->data;
	const uint32_t id = cmd->cdw12.id;
	const bool rae = cmd->cdw12.rae;
#ifdef DEBUG
	printf("Status requested, id: %d, rae: %d\n", id, rae);
#endif
	if(id >= accelerators.size()) {
		printf("Invalid Accelerator ID! (%u)\n", id);
		return;
	}

	if(!rae)
		calculate_status(id);

	if(recv->buf_len <= sizeof(heads[0])) 
		memcpy(buf, &heads[id], recv->buf_len);
}
