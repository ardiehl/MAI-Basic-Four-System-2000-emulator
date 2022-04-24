#######################################
# Makefile for eagle tests
#
# Armin Diehl <ad@ardiehl.de> 04/2015
#######################################

CFLAGS=-Wall
ifneq ($(ARCH),local)
SRCS=eagle-sys.c
CFLAGS+=-Deagle
endif
SRCS+=eagletest.c util.c
PRG=eagletest
#LDFLAGS=-nostartfiles
CFLAGS+=-I.-O3
# set to 1 for genarting list-files
GENASM=1
# set to 1 for verbose linking
VERBOSE=0
# 1 for generating a linker map file
MAP=1
# ldscript to use
LDSCRIPT68K=eagle.ld
INSTALLINTO=~/prj/68000-emus/eagleemu
ifeq ($(ARCH),local)
OBJDIR=obj/
else
OBJDIR=obj/$(ARCH)/
endif

OBJS=$(patsubst %.c,$(OBJDIR)%.o,$(patsubst %.S,$(OBJDIR)%.o,$(SRCS)))
LSTS=$(patsubst %.c,$(OBJDIR)%.lst,$(patsubst %.S,$(OBJDIR)%.o,$(SRCS)))


#==============================================================================

ifeq ($(ARCH),local)
CC=gcc
OBJCOPY=objcopy
SIZE=size
else
CC=$(ARCH)-gcc
OBJCOPY=$(ARCH)-objcopy
SIZE=$(ARCH)-size
ARCH=m68k-elf
MCPU=m68000
#MARCH=m68000
LDSCRIPT=$(LDSCRIPT68K)
endif

PREFIX=$(ARCH)-
ifeq ($(PREFIX),-)
PREFIX=
endif



ifeq ($(GENASM),1)
#CFLAGS+= -S
CFLAGS+= -g -Wa,-adhlns=$(OBJDIR)$(<:.c=.lst)
endif

ifeq ($(VERBOSE),1)
LDFLAGS+= -Xlinker -t
endif

ifeq ($(MAP),1)
MAPFILE=$(OBJDIR)$(PRG).map
LDFLAGS+= -Xlinker -Map=$(MAPFILE)
endif

ifdef LDSCRIPT
LDFLAGS+= -T $(LDSCRIPT)
endif

ifdef MCPU
AFLAGS+=-$(MCPU)
endif

ifdef MARCH
AFLAGS+=-march=$(MARCH)
endif


# make object files from C source files
$(OBJDIR)%.o:	%.c
	@echo "compiling $<"
	$(CC) $(AFLAGS) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o:	%.S
	@echo "assembling $<"
	$(CC) $(AFLAGS) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

#crt0.o:		crt0.S asm.h

both:
	make all
	make ARCH=local all

all: check $(OBJDIR)$(PRG).srec

check:
	@mkdir -p $(OBJDIR)


ifdef INSTALLINTO
install: $(PRG)
	@echo "installing $< to $(INSTALLINTO)"
	cp $< $(INSTALLINTO)/$<
endif

$(OBJDIR)$(PRG): $(OBJS)
	$(CC) $(AFLAGS) $(LDFLAGS) -o $(OBJDIR)$(PRG) $(OBJS)

$(OBJDIR)$(PRG).srec: $(OBJDIR)$(PRG)
ifneq ($(ARCH),local)
	$(OBJCOPY) -O srec $< $<.srec
endif
	$(SIZE) $<

clean_arch:
	@echo "cleaning"
	rm -f $(OBJS) $(LSTS) $(OBJDIR)$(PRG) $(OBJDIR)$(PRG).srec $(MAPFILE)
	
clean:
	@make ARCH=local clean_arch
	@make ARCH=m68k-elf clean_arch

local:
	make ARCH=local
