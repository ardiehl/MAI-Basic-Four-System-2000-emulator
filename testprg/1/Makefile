ARCH=m68k-elf
CC=$(ARCH)-gcc
SRCS=crt0.S test.c
OBJS=$(patsubst %.c,%.o,$(patsubst %.S,%.o,$(SRCS)))
LDFLAGS=-lc

# make object files from C source files
%.o:	%.c
	@echo "compiling $<"
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o:	%.S
	@echo "assembling $<"
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

crt0.o:		crt0.S asm.h

all: $(OBJS)

test: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o test $(OBJS)

clean:
	echo "cleaning $(OBJS)"
	rm -f $(OBJS)
