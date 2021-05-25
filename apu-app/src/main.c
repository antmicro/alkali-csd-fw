#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

int rpmsg_init(void);

struct payload {
	uint32_t id;
	uint32_t len;
	uint32_t addr;
};

int main(int argc, char *argv[])
{
	struct payload initial_msg = {1234, 0, 0};
	struct payload recv_msg = {0};
	int fd = rpmsg_init();

	if(fd < 0)
		return fd;

	write(fd, &initial_msg, sizeof(initial_msg));

	while(recv_msg.id != 0x11) {
		int bytes = read(fd, &recv_msg, sizeof(recv_msg));
		if(bytes == sizeof(recv_msg)) {
			printf("command received, id: %d, len: %d, addr: 0x%08x\n", recv_msg.id, recv_msg.len, recv_msg.addr);
		}
	}

	return 0;
}
