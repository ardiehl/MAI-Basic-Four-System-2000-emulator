CFLAGS= -std=c89 -g -O0 -W -I../ws-compat -DUSESTDCLIB
WCDIR = ../ws-compat
WCHDR = $(WCDIR)/ws_libc.h $(WCDIR)/std.h
WC    = $(WCDIR)/ws_libc.c
WOBJ  = ws_libc.o
LD    = $(CC)

OBJS = lib.o $(WOBJ)
PROG = lib
LHDR = 

$(PROG):	$(OBJS)
	echo "Linking $(PROG)"
	$(LD) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBLNK) -o $(PROG)

$(WOBJ):  $(WCHDR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(WC)

lib.o:  lib.c $(LHDR) $(WCHDR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) lib.c

clean:
	rm $(OBJS) $(PROG)

