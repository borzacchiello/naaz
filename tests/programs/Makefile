SRCS   = $(wildcard *.c)
PROGS  = $(patsubst %.c,%.elf,$(SRCS))
CFLAGS = -O0

all: $(PROGS)

%.elf: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS)
