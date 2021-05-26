#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

int rpmsg_init(void);

struct __attribute__((__packed__)) payload {
	uint32_t id;
	uint32_t len;
	uint32_t priv;
	uint32_t data[];
};

int main(int argc, char *argv[])
{
	struct payload initial_msg = {1234, 0};
	struct payload recv_msg = {0};
	int fd = rpmsg_init();

	if(fd < 0)
		return fd;

	write(fd, &initial_msg, sizeof(initial_msg));

	for(;;) {
		int bytes = read(fd, &recv_msg, sizeof(recv_msg));
		if(bytes == sizeof(recv_msg)) {
			struct payload ack_msg = {0};
			printf("command received, id: %d, len: %d, priv: %08x\n", recv_msg.id, recv_msg.len, recv_msg.priv);
			ack_msg.id = 0x20;
			ack_msg.priv = recv_msg.priv;
			bytes = write(fd, &ack_msg, sizeof(ack_msg));
			if(bytes != sizeof(ack_msg))
				printf("Failed to send ACK: %d\n", bytes);
		}
	}

	return 0;
}
