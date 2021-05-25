CC=$(CROSS_COMPILE)gcc
OBJS=$(patsubst %.c,%.o,$(wildcard src/*.c))
CFLAGS=-Wall -Werror
OUT=apu-app

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(OUT) $(OBJS)

.PHONY: clean
