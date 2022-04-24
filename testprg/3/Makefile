ARCH=m68k-elf
CC=$(ARCH)-gcc
SRCS=eagle-sys.c test.c
OBJS=$(patsubst %.c,%.o,$(patsubst %.S,%.o,$(SRCS)))
ASMS=$(patsubst %.c,%.s,$(patsubst %.S,%.o,$(SRCS)))
LDFLAGS=
CFLAGS=-I.
PRG=test
GENASM=1

ifeq ($(GENASM),1)
CFLAGS+= -S
endif


# make object files from C source files
%.o:	%.c
	@echo "compiling $<"
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o:	%.S
	@echo "assembling $<"
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

#crt0.o:		crt0.S asm.h

all: $(OBJS)

$(PRG) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PRG) $(OBJS)

clean:
	echo "cleaning $(OBJS) $(ASM) $(PRG)"
	rm -f $(OBJS) $(ASM) $(PRG)
