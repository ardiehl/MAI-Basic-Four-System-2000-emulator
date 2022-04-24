
PROJECT  = eagletest

CSRC     = $(wildcard *.c)
CXXSRC   = $(wildcard *.cpp)
ASRC     = $(wildcard *.S)

#SUBDIRS  = Drivers Core

ifneq ($(BUILD_TARGET),EAGLE)
OPTIMIZE = 0

INC     = .
OUTDIR  = lbin
CHIP    = -m32
LDFLAGS  = $(FLAGS) -Wl,--as-needed,--gc-sections

else
OPTIMIZE = s
CDEFS	= EAGLE
INC      = .
OUTDIR   = bin
OSRC     =
CHIP     = -mcpu=68010 -m68010 -mtune=68010 -mno-bitfield
#ARCH     = m68k-linux-gnu
ARCH     = m68k-elf
PREFIX   = $(ARCH)-
#LDFLAGS  = $(FLAGS) -Wl,--as-needed,--gc-sections,-T,eagle.ld
LDFLAGS  = $(FLAGS) -Wl,-T,eagle.ld

endif

CC       = $(PREFIX)gcc
CXX      = $(PREFIX)g++
OBJCOPY  = $(PREFIX)objcopy
OBJDUMP  = $(PREFIX)objdump
AR       = $(PREFIX)ar
SIZE     = $(PREFIX)size
READELF  = $(PREFIX)readelf
# You MUST link with G++ if you have even one C++ source file in the project
# If you have no C++, then feel free to link with gcc which gives a significant reduction in included library code
LINK     = $(PREFIX)gcc

MKDIR    = mkdir
RMDIR    = rmdir
RM       = rm -f
CP       = cp -f
SREC_CAT = srec_cat


#DEBUG_MESSAGES

FLAGS    = -O$(OPTIMIZE) -Wall $(CHIP) -Wno-char-subscripts -ggdb -fstack-check
FLAGS   += $(patsubst %,-I%,$(INC))
FLAGS   += $(patsubst %,-D%,$(CDEFS))
CFLAGS   = $(FLAGS) -std=gnu99 -pipe -fno-builtin-printf -fno-builtin-fprintf -fno-builtin-vfprintf -fno-builtin-puts
ASFLAGS  = $(FLAGS)
CXXFLAGS = $(FLAGS) -fno-rtti -fno-exceptions -std=gnu++0x

LDFLAGS += $(patsubst %,-L%,$(LIBRARIES))
LDFLAGS += -Wl,-Map=$(OUTDIR)/$(PROJECT).map
#-nostdlib

OBJ      = $(patsubst %,$(OUTDIR)/%,$(notdir $(CSRC:.c=.o) $(CXXSRC:.cpp=.o) $(ASRC:.S=.o)))

VPATH    = . $(patsubst %/inc,%/src,$(INC)) $(dir $(NXPSRC)) $(dir $(USBSRC)) $(dir $(UIPSRC)) $(dir $(LWIPSRC))

.PHONY: all clean program upload size functions functionsizes

.PRECIOUS: $(OBJ)

all: $(OUTDIR) $(OUTDIR)/$(PROJECT).elf $(OUTDIR)/$(PROJECT).hex $(OUTDIR)/$(PROJECT).srec $(OUTDIR)/$(PROJECT).dis size

clean:
	@echo "  RM    " ".o"
	@$(RM) $(OBJ) $(OBJ:%.o=%.lst)

	@echo "  RM    " "build/"$(PROJECT)".*"
	@$(RM) $(OUTDIR)/$(PROJECT).bin $(OUTDIR)/$(PROJECT).hex $(OUTDIR)/$(PROJECT).srec $(OUTDIR)/$(PROJECT).elf $(OUTDIR)/$(PROJECT) $(OUTDIR)/$(PROJECT).map $(OUTDIR)/$(PROJECT).sym $(OUTDIR)/$(PROJECT).dis

#	@echo "  RM    " "build/"
#	@$(RMDIR) $(OUTDIR); true


size: $(OUTDIR)/$(PROJECT).elf
	@$(SIZE) $<

functions: $(OUTDIR)/$(PROJECT).elf
	@$(READELF) -s $^ | perl -e 'for (<>) { /^\s+(\d+):\s*([0-9A-F]+)\s+(\d+)/i && do { s/^\s+//; push @symbols, [ split /\s+/, $$_ ]; }; }; for (sort { hex($$a->[1]) <=> hex($$b->[1]); } @symbols) { printf "0x%08s: [%4d] %7s %s\n", $$_->[1], $$_->[2], $$_->[3], $$_->[7] if ($$_->[2]) && (hex($$_->[1]) < 0x10000000); }'

functionsizes: $(OUTDIR)/$(PROJECT).elf
	@$(READELF) -s $^ | perl -e 'for (<>) { /^\s+(\d+):\s*([0-9A-F]+)\s+(\d+)/i && do { s/^\s+//; push @symbols, [ split /\s+/, $$_ ]; }; }; for (sort { $$a->[2] <=> $$b->[2]; } @symbols) { printf "0x%08s: [%4d] %7s %s\n", $$_->[1], $$_->[2], $$_->[3], $$_->[7] if ($$_->[2]) && (hex($$_->[1]) < 0x10000000); }'

$(OUTDIR):
	@$(MKDIR) $(OUTDIR)

$(OUTDIR)/%.bin: $(OUTDIR)/%.elf
	@echo "  COPY  " $@
	@$(OBJCOPY) -O binary $< $@

$(OUTDIR)/%.hex: $(OUTDIR)/%.elf
	@echo "  COPY  " $@
	@$(OBJCOPY) -O srec --srec-forceS3 $< $@

$(OUTDIR)/%.srec: $(OUTDIR)/%.hex
	@echo "  SREC  " $@
	@$(SREC_CAT) -Output $@ -Motorola -Address_Length=3 -Line_Length=240 $<

$(OUTDIR)/%.sym: $(OUTDIR)/%.elf
	@echo "  SYM   " $@
	@$(OBJDUMP) -t $< | perl -ne 'BEGIN { printf "%6s  %-40s %s\n", "ADDR","NAME","SIZE"; } /([0-9a-f]+)\s+(\w+)\s+O\s+\.(bss|data)\s+([0-9a-f]+)\s+(\w+)/ && printf "0x%04x  %-40s +%d\n", eval("0x$$1") & 0xFFFF, $$5, eval("0x$$4")' | sort -k1 > $@

$(OUTDIR)/%.dis: $(OUTDIR)/%.elf
	@echo "  DIS   " $@
	@$(OBJDUMP) -S $(@:.dis=.elf)  > $@

$(OUTDIR)/%.elf: $(OBJ)
	@echo "  LINK  " $@
	@$(LINK) $(OSRC) -Wl,-Map=$(@:.elf=.map) -o $@ $^ $(LDFLAGS)
	@$(CP) $@ $(@:.elf=)

$(OUTDIR)/%.o: %.c Makefile
	@echo "  CC    " $@
	@$(CC) $(CFLAGS) -Wa,-adhlns=$(@:.o=.lst) -c -o $@ $<

# $(OUTDIR)/%.o: %.cpp
# 	@echo "  CXX   " $@
# 	@$(CXX) $(CXXFLAGS) -Wa,-adhlns=$(@:.o=.lst) -c -o $@ $<

$(OUTDIR)/%.o: %.S Makefile
	@echo "  AS    " $@
	@$(CC) $(ASFLAGS) -Wa,-adhlns=$(@:.o=.lst) -c -o $@ $<

Debug:	all

cleanDebug:	clean
