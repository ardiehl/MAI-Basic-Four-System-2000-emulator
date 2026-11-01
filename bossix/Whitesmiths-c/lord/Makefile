CFLAGS= -std=c89 -g -O0 -W -I../ws-compat -DUSESTDCLIB
WCDIR = ../ws-compat
WCHDR = $(WCDIR)/ws_libc.h $(WCDIR)/std.h
WC    = $(WCDIR)/ws_libc.c
WOBJ  = ws_libc.o
LD    = $(CC)

OBJS = lord.o $(WOBJ)
PROG = lord
LHDR = 

$(PROG):	$(OBJS)
	echo "Linking $(PROG)"
	$(LD) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBLNK) -o $(PROG)

$(WOBJ):  $(WCHDR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(WC)

lord.o:  lord.c $(LHDR) $(WCHDR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) lord.c

clean:
	rm $(OBJS) $(PROG)

