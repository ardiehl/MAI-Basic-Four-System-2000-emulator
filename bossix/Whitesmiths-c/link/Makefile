CFLAGS= -std=c89 -g -O0 -W -Wno-incompatible-pointer-types -I../ws-compat -DUSESTDCLIB
WCDIR = ../ws-compat
WCHDR = $(WCDIR)/ws_libc.h $(WCDIR)/std.h
WC    = $(WCDIR)/ws_libc.c
WOBJ  = ws_libc.o
LD    = $(CC)

OBJS = link1.o link2.o $(WOBJ)
PROG = link
LHDR = link.h

$(PROG):	$(OBJS)
	echo "Linking $(PROG)"
	$(LD) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBLNK) -o $(PROG)

$(WOBJ):  $(WCHDR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(WC)

link1.o:  link1.c $(LHDR) $(WCHDR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) link1.c

link2.o:  link2.c  $(LHDR) $(WCHDR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) link2.c

clean:
	rm $(OBJS) $(PROG)

