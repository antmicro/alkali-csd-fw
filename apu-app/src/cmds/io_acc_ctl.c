#include "cmd.h"
#include "nvme.h"
#include "acc.h"

#include <cstdio>
	 
typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	uint32_t cdw10;
	uint32_t cdw11;
	uint32_t cdw12;
	uint32_t cdw13;
	uint32_t cdw14;
} cmd_sq_t;

void io_cmd_acc_ctl(payload_t *recv)
{
	cmd_sq_t *cmd = (cmd_sq_t*)recv->data;
	const uint32_t id = cmd->cdw14;
	const uint32_t op = cmd->cdw13;
	const uint32_t fw_id = cmd->cdw14;
#ifdef DEBUG
	printf("IO CTL id: %d, op: %d\n", id, op);
#endif
	if(id >= accelerators.size()) {
		printf("Invalid accelerator ID! (%d)\n", id);
		return;
	}

	Acc *a = accelerators[id];

	switch(op) {
		case ACC_IO_OP_RESET:
			accelerators[id] = new Acc(id);
			delete a;
			break;
		case ACC_IO_OP_START:
			a->start();
			break;
		case ACC_IO_OP_STOP:
			a->stop();
			break;
		case ACC_IO_OP_SET_FW:
			a->addFirmware(*fw_map[fw_id]);
			break;
		default:
			printf("Unsupported operation! (%d)\n", op);
	}
}
