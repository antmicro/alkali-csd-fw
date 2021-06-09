CC=$(CROSS_COMPILE)gcc
SRCS=$(wildcard src/*.c)
SRCS+=$(wildcard src/cmds/*.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))
CFLAGS=-Wall -Werror -static
OUT=apu-app

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(OUT) $(OBJS)

.PHONY: clean
