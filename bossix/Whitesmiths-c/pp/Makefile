CFLAGS= -std=c89 -g -O0 -W -Wno-incompatible-pointer-types -I../ws-compat -DUSESTDCLIB
WCDIR = ../ws-compat
WCHDR = $(WCDIR)/ws_libc.h $(WCDIR)/std.h
WC    = $(WCDIR)/ws_libc.c
WOBJ  = ws_libc.o
LD    = $(CC)

OBJS = p0eval.o p0lex.o p0main.o p0subs.o $(WOBJ)
PROG = pp
LHDR = int012.h int01.h int0.h

$(PROG):	$(OBJS)
	echo "Linking $(PROG)"
	$(LD) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBLNK) -o $(PROG)

$(WOBJ):  $(WCHDR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(WC)

p0eval.o:  p0eval.c $(LHDR) $(WCHDR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) p0eval.c

p0lex.o:  p0lex.c  $(LHDR) $(WCHDR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) p0lex.c

p0subs.o:  p0subs.c  $(LHDR) $(WCHDR)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) p0subs.c

clean:
	rm $(OBJS) $(PROG)


