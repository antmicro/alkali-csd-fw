CC=$(CROSS_COMPILE)g++
SRCS=$(wildcard src/*.c)
SRCS+=$(wildcard src/cmds/*.c)
SRCS+=$(wildcard src/vm/*.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))
CFLAGS=-Wall -Werror -static -Isrc --std=c++11
LDFLAGS=-Lsrc
LDLIBS=-lubpf -lpthread
OUT=apu-app

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(OUT) $(OBJS)

.PHONY: clean
