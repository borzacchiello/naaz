SRCS  = $(wildcard *.c)
PROGS = $(patsubst %.c,%.elf,$(SRCS))

all: $(PROGS)

%.elf: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS)
