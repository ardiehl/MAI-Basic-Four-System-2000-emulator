PRJ=sensew
#ASM=asm68k -s
#ASM=68kasm
#ASMFLAGS=-l
ASM=/mpeg/prj/68000utils/asl-current/asl
ASMFLAGS=-cpu 68010 -L -q -gnuerrors
P2HEX=/mpeg/prj/68000utils/asl-current/p2hex
P2HEXFLAGS=-F Moto -M 2 -r \$$0000-\$$ffffffff
all: $(PRJ).srec

build: clean all

clean:
	rm -f $(PRJ).srec $(PRJ).p $(PRJ).x68

$(PRJ).srec : $(PRJ).asm68 $(PRJ).p
#	$(ASM) $(ASMFLAGS) $(PRJ).x68



%.p : %.asm68
	$(ASM) $(ASMFLAGS) $<

$(PRJ).srec : $(PRJ).p
	$(P2HEX) $< $(PRJ).srec $(P2HEXFLAGS)
	cp $(PRJ).srec ../../68000emus/eagleemu

x:
	sed 's/;/*/g' $(PRJ).asm68 >$(PRJ).x68
	unix2dos $(PRJ).x68
