/* routines that are included in the Whiteshmits but not in a standard c library
 * AD 10/2026

 AD: Parts taken from the feely available Witheshmis runtime library available at
 https://github.com/hansake/Whitesmiths-C-compiler

 Therefore non-commerecial purposes only

 */
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <std.h>
#undef fread
#undef fwrite

TEXT *alloc (BYTES nBytes, void *link) {
	char *p = malloc(nBytes);
	if (!p) return p;
	if (link)
		*(void **)p = link;
	return p;
}


TEXT *buybuf(TEXT *s, BYTES n) {
	TEXT *res = malloc(n);
	if (res) memcpy(res,s,n);
	return res;
}


BYTES lenstr (TEXT * s) {
	return strlen (s);
}

BOOL cmpbuf(TEXT * s1, TEXT * s2, BYTES n) {
	if (memcmp (s1,s2, n) == 0) return 1;
	return 0;
}

BYTES scnstr (TEXT *s, TEXT c) {
	char *p = strchr (s, c);
	return p - s;
}


/* is s2 a prefix of s1 */
BOOL prefix(TEXT *s1, TEXT *s2) {
	while (*s2)
		if (*s1++ != *s2++) return (NO);
	return (YES);
}


int create (TEXT * fname, COUNT mode, BYTES rsize) {
	mode_t mo = 0;

	if (rsize != 1) {
		fprintf(stderr,"create: only record size 1 supported\n");
		exit(1);
	}


	switch (mode) {
		case READ: break;
		case WRITE: mo = O_CREAT | O_TRUNC | O_RDWR; break;
		case UPDATE: mo = O_CREAT | O_RDWR; break;
		default: return -1;
	}
	int res = open (fname, mo, 0644);
	/* printf("create %s, mode: %d, mo: %d, res: %d, Errno:%d, %s\n",fname,mode,mo,res,errno,strerror(errno)); */
	return res;
}


BYTES cpybuf (TEXT *s1,TEXT *s2, BYTES n) {
	memcpy (s1,s2,n);
	return n;
}


void putstr (int fd, ...) {
	va_list ap;
	va_start (ap, fd);

	char *p;
	while (p = va_arg (ap, char *)) {
		write(fd,p,strlen(p));
	}
}


TEXT * cpystr (TEXT *target, ...) {
	va_list ap;
	va_start (ap, target);
	*target = '\0';
	char * end;

	char *p;
	while (p = va_arg (ap, char *)) {
		end = strcat(target,p);
	}
	return end;
}

/* TODO: should be a macro in std ? */
/*
int iswhite(char ch) {
    return (ch == ' ' || ch == '\t' || ch == '\r');
}
*/
BYTES btol(TEXT *s, BYTES n, LONG *l, COUNT base) {
	FAST COUNT dig;
	BOOL minus;
	LONG val;
	TEXT *is;

	val = 0;
	is = s;
	minus = NO;
	for(; 0 < n && iswhite(*s); --n, ++s)
			;
	if (0 < n)
			if (*s == '-')
					{
					minus = YES;
					++s;
					--n;
					}
			else if (*s == '+')
					{
					++s;
					--n;
					}
	if (base == 16 && 2 <= n)
			if (*s == '0' && tolower(s[1]) == 'x')
					{
					s += 2;
					n -= 2;
					}
	for (; 0 < n; --n, ++s)
			{
			if (isdigit(*s))
					dig = *s - '0';
			else if (isalpha(*s))
					{
					dig = tolower(*s) - ('a' - 10);
					if (base <= dig)
							break;
					}
			else
					break;
			val = val * base + dig;
			}
	*l = ((minus) ? -val : val);
	if (0 < n && tolower(*s) == 'l')
			++s;
	return (s - is);
}

/*	COLLECT FLAGS FROM COMMAND ARGUMENT LIST
 *	copyright (c) 1981 by Whitesmiths, Ltd.
 */
/*#include "stdtype.h" */

struct st {
	BYTES ntop;
	TEXT  *aname[255];
};

struct slong {
	BYTES ntop;
	LONG  along[255];
};

union ap {
	TEXT  *tx;
	TEXT  ch;
	BYTES ival;
	LONG  lval;
	BOOL bval;
	struct st stp;
	struct slong slongp;
};

#define BRKLINE 70

/*	put a char with wraparound
 */
LOCAL COUNT putcw(fd, pc, i)
	FAST int fd;
	FAST TEXT *pc;
	FAST COUNT i;
	{
	if (BRKLINE < i && (*pc == ' ' || *pc == '\t'))
		{
		write(fd, "\n\t", 2);
		return (4);
		}
	else
		{
		write(fd, pc, 1);
		return (i + 1);
		}
	}

/*	write standard "usage" text to STDERR
 */
COUNT usage(TEXT * msg)	{
	IMPORT TEXT *_pname;
	FAST COUNT r;

	r = 0;
	putstr(STDERR, "usage: ", _pname, " ", msg, NULL);
	if (msg && msg[(r = strlen(msg)) - 1] == '\n')
		exit(1);
	return (r + strlen(_pname) + 8);
}




/*	print usage and flags, and die
 */
LOCAL VOID _error(TEXT *mesg, TEXT *fmt) {
	FAST COUNT i;

	i = usage(NULL);
	for (; *mesg; ++mesg)
		if (*mesg != 'F')
			i = putcw(STDERR, mesg, i);
		else
			{
			i += write(STDERR, "-[", 2);
			for ( ; *fmt != ':'; ++fmt)
				if (*fmt == ',' && fmt[1] != ':')
					i = putcw(STDERR, " ", i);
				else if (*fmt == '>')
					{
					i += write(STDERR, "^", 1);
					if (fmt[1] != ':')
						i = putcw(STDERR, " ", i);
					}
				else
					i += write(STDERR, fmt, 1);
			i += write(STDERR, "]", 1);
			}
	write(STDERR, "\n", 1);
	exit(1);
}



/*	collect flags from command argument list
 */
TEXT *getflags(BYTES *pac, TEXT ***pav, TEXT *fmt, ... )
	/*union ap *r4,*r3,*r2,*stack;*/
	{
	FAST TEXT *f, *q, *r;
	BOOL out, suc, next;
	COUNT base, s, v;
	LONG lo;
	va_list vap;

	typedef enum {
	    pt_none, pt_tx, pt_ch, pt_ival, pt_lval, pt_stp, pt_slongp
	} parType_t;

	union ap val,*p;
	COUNT prcnt;
	parType_t parType = pt_none;

	for (++*pav; *pac && --*pac; ++*pav)
		{
		q = **pav;
		if (!strcmp(q, "-help")) {
		    TEXT *msg = "F";
            if (*(msg = fmt + scnstr(fmt, ':'))) msg++;

			_error(msg,fmt);
		} else if (!strcmp(q, "--"))
			{
			--*pac;
			++*pav;
			break;
			}
		else if (!strcmp(q, "-") || (*q != '-' && *q != '+'))
			break;
		if (*q == '-')
			++q;

		while (*q) {
            va_start (vap, fmt); /* p = &pr4,  */
            p = va_arg (vap, void *);
            /* p=parameter, f=fmt */
            for (f=fmt, next=out=suc=NO, prcnt=0, r=q;!next; ++f)
				switch (*f)	{
				case '*':
					suc = YES;
					val.tx = (*r) ? r : (0 < --*pac) ? *++*pav : r;
					parType = pt_tx;
					r = val.tx + strlen(val.tx);
					break;
				case '?':
					suc = YES;
					val.ch = *r;
					parType = pt_ch;
					if (*r)
						++r;
					break;
				case '#':
					if (!*r)
						r = (0 < --*pac) ? *++*pav : r;
					suc = ((s = (*r == '-' || *r == '+')) || isdigit(*r));
					if (*r != '0')
						base = 10;
					else if (tolower(r[1]) == 'x')
						base = 16;
					else
						base = 8;
					v = btol(r, strlen(r), &lo, base);
					if (!suc || v <= (s) ? 1 : 0 || *(r += v))
						if (f[v = scnstr(f, ':')])
							_error(&f[v + 1], fmt);
						else
							return (q);
					if (f[1] == '#' && f[2] != '>')
						{
						p->lval = lo;
						++f;
						out = YES;
						parType = pt_lval;
						}
					if (f[1] == '#' && f[2] == '>')
					{
						val.lval = lo;
						++f;
						parType = pt_slongp;

					}
					else {
						val.ival = (BYTES) lo;
						parType = pt_ival;
					}
					break;
				case ',':
				case ':':
				case '\0':
					if (suc)
						{
						if (!out) {
                            switch(parType) {
                                case pt_none:
                                    /*error("getflags: parameter type not set before set",NULL);*/
                                    p->bval = val.ival;
                                    break;
                                case pt_tx: /* string pointer */
                                    p->tx = val.tx;
                                    break;
                                case pt_ch:
                                    p->ch = val.ch;
                                    break;
                                case pt_ival:
                                    p->ival = val.ival;
                                    break;
                                case pt_lval:
                                    p->lval = val.lval;
                                    break;
                                case pt_stp:
                                    error("getflags: pstp",NULL);
                            }
                            parType = pt_none;
							/* p->ival = val.ival; */
						} else
							out = NO;
						q = r;
						next = YES;
						}
					else if (!*f)
						return (q);
					else if (*f == ':')
						_error(++f, fmt);
					else
						{
						/* if (++prcnt == 3) p = &stack; else ++p;  next arg */
						p = va_arg (vap, void *);
						r = q;
						}
					break;

				case '>':
					if (suc)
						if (0 < p->stp.ntop)
							{
                            if (parType == pt_tx)
                                p->stp.aname[--p->stp.ntop] = val.tx;
                            else
                                p->slongp.along[--p->slongp.ntop] = val.lval;

							q = r;
							next = YES;
							}
						else
							if (f[v = scnstr(f, ':')])
								_error(&f[v + 1], fmt);
							else
								return (q);
					else
						{
						p = va_arg (vap, void *);
						r = q;
						}
					break;
				default:
					if (*f == *r)
						{
						suc = YES;
						val.ival = YES;
						++r;
						}
					else
						{
						suc = NO;
						while (*f != ',' && *f != '>' && *f != ':' && *f)
							++f;
						--f;
						}
					break;
				}
		}


/*
			for (f=fmt, next=out=suc=NO, p = &pr4, prcnt=0, r=q;!next; ++f)
				switch (*f)
					{
				case '*':
					suc = YES;
					val.tx = (*r) ? r : (0 < --*pac) ? *++*pav : r;
					r = val.tx + strlen(val.tx);
					break;
				case '?':
					suc = YES;
					val.ch = *r;
					if (*r)
						++r;
					break;
				case '#':
					if (!*r)
						r = (0 < --*pac) ? *++*pav : r;
					suc = ((s = (*r == '-' || *r == '+')) || isdigit(*r));
					if (*r != '0')
						base = 10;
					else if (tolower(r[1]) == 'x')
						base = 16;
					else
						base = 8;
					v = btol(r, strlen(r), &lo, base);
					if (!suc || v <= (s) ? 1 : 0 || *(r += v))
						if (f[v = scnstr(f, ':')])
							_error(&f[v + 1], fmt);
						else
							return (q);
					if (f[1] == '#' && f[2] != '>')
						{
						(*p)->lval = lo;
						++f;
						out = YES;
						}
					else
						val.ival = (BYTES) lo;
					break;
				case ',':
				case ':':
				case '\0':
					if (suc)
						{
						if (!out)
							(*p)->ival = val.ival;
						else
							out = NO;
						q = r;
						next = YES;
						}
					else if (!*f)
						return (q);
					else if (*f == ':')
						_error(++f, fmt);
					else
						{
						if (++prcnt == 3)
							p = &stack;
						else
							++p;
						r = q;
						}
					break;
				case '>':
					if (suc)
						if (0 < (*p)->stp.ntop)
							{
							(*p)->stp.aname[--(*p)->stp.ntop] = val.tx;
							q = r;
							next = YES;
							}
						else
							if (f[v = scnstr(f, ':')])
								_error(&f[v + 1], fmt);
							else
								return (q);
					else
						{
						if (++prcnt == 3)
							p = &stack;
						else
							++p;
						r = q;
						}
					break;
				default:
					if (*f == *r)
						{
						suc = YES;
						val.ival = YES;
						++r;
						}
					else
						{
						suc = NO;
						while (*f != ',' && *f != '>' && *f != ':' && *f)
							++f;
						--f;
						}
					break;
				}

*/

		}
	return (NULL);
	}




int getfiles(int *pac, TEXT ***pav, int dfd, int efd)
/*
        FAST ARGINT *pac;
        FAST TEXT ***pav;
        FILE dfd, efd; */
        {
        int fd;

        if (*pac < 0)
                fd = -1;
        else if (*pac == 0 || cmpstr(**pav, "-"))
                fd = dfd;
        else if ((fd = open(**pav, READ, 0)) < 0)
                fd = efd;
        ++*pav;
        if (--*pac <= 0)
                *pac = -1;
        return (fd);
        }

#define	NAMSIZE	64

/*	linked list structure
 */
#define LNKLST	struct lnklst
typedef struct lnklst {
	struct lnklst *next;
	TEXT *buf;
};

TEXT *wsfree(TEXT *pcell, TEXT *link) {
	if (pcell) free(pcell);
	return link;
}

BOOL getin(BYTES *pac, TEXT ***pav) {
	BOOL eof, success;
	BYTES i;
	COUNT n;
	LNKLST *head, *p, **qb;
	TEXT buf[NAMSIZE], *pbuf, **s;

	eof = NO;
	success = YES;
	*pac = 0;
	head = NULL;
	qb = &head;
	while (!eof) {
		for (i = 0, pbuf = (TEXT *)&buf; i < NAMSIZE &&
			0 < (n = read(STDIN, pbuf, 1)) && *pbuf != '\n'; ++i, ++pbuf);
		eof = (n <= 0);
		if (!i);
		else if (i == NAMSIZE)	{
			buf[NAMSIZE - 1] = '\0';
			success = remark("argument too long: ", buf);
			for (pbuf = (TEXT *)&buf; 0 < (n = read(STDIN, pbuf, 1)) &&
				*pbuf != '\n'; )
				;
			eof = (n <= 0);
		} else {
			p = (LNKLST *)alloc(sizeof(LNKLST), NULL);
			*qb = p;
			qb = &p->next;
			buf[i] = '\0';
			p->buf = buybuf(buf, i + 1);
			++*pac;
			}
		}
	s = alloc(sizeof(**pav) * (*pac + 1), NULL);
	*pav = s;
	for (p = head; p; p = (LNKLST *)wsfree((TEXT *)p, (TEXT *)p->next))
		*s++ = p->buf;
	*s = NULL;
	return (success);
}



BYTES putl(FILE *pfio, TEXT *s, BYTES n) {
	/* TODO: raise error condition */
	int res = fwrite (s,1,n,pfio);
	fflush(pfio);
	if (res < 0) {
		fprintf(stderr,"putl %d bytes failed with %d %s\n",n,errno,strerror(errno));
		exit(1);
	}
	return res;
}

BOOL cmpstr (TEXT *s1, TEXT * s2) {
	return (strcmp (s1,s2) == 0);
}

BYTES itob(TEXT *is, ARGINT n, COUNT base) {
	int u;
	char *s;

	s = is;
	if (n < 0 && base <= 0)	{
		u = -n;
		*s++ = '-';
	}
	else
		u = n;
	if (base == 0)
		base = 10;
	else if (base < 0)
		base = -base;
	if (base <= u)
		s += itob(s, u / base, base);
	*s = (u % base) + '0';
	if ('9' < *s)
			*s =+ ('a' - ('9' + 1));
	return (s - is + 1);
}




static onexit_registered = 0;


#include <stdlib.h>


static onexit_t _onexit = onexit;

/*      exit to system
 */
static void internal_exit() {
    while (_onexit && _onexit != onexit)
        _onexit = (onexit_t)(*_onexit)((exit_func_t)NULL);
}

/*      register function to call on exit
 */
exit_func_t onexit(exit_func_t new_func) {
    onexit_t p;
    if (!onexit_registered) {
		atexit(&internal_exit);
		onexit_registered++;
	}

    p = _onexit;
    _onexit = (onexit_t)new_func;
    return (exit_func_t)p;
}



exit_func_t onintr(exit_func_t new_func) {
	/* TODO */
}

/* gtc was a macro using FIO internals */
COUNT gtc (FILE *f) {
	unsigned char c;
	int res = fread(&c,1,1,f);
	if (res) return c;
	return -1;
}

COUNT lstoi(UTINY *s) {
	COUNT wo = s[1] << 8 | s[0];
	return (wo);
}


/* Get files from command list */
/*      GET BINARY OR TEXT FILES FROM COMMAND LIST
 *      copyright (c) 1980 by Whitesmiths, Ltd.
 */


int getbfiles(ARGINT *pac, TEXT ***pav, int dfd, int efd, BYTES rsize) {
	int fd;

	if (*pac < 0)
		fd = -1;
	else if (*pac == 0 || cmpstr(**pav, "-"))
		fd = dfd;
	else if ((fd = open(**pav, READ, rsize)) < 0)
		fd = efd;
	++*pav;
	if (--*pac <= 0)
		*pac = -1;
	return (fd);
}

/* remark - print non-fatal error message */
VOID remark(TEXT *s1, TEXT *s2) {
	if (s1) fprintf(stderr,s1);
	if (s2) fprintf(stderr,s2);
	fprintf(stderr,"\n");
}

VOID error(TEXT *s1, TEXT *s2) {
    remark(s1,s2);
    exit(1);
}


/* get a text line from stdin*/
BYTES getlin(TEXT *s, BYTES n) {
	char *read = fgets(s, n, stdin);
	if (!read) return 0;
	return strlen(read);
}


LOCAL VOID _sort(lv, uv, ordf, excf, base)
	COUNT lv, uv;
	COUNT (*ordf)();
	VOID (*excf)();
	TEXT *base;
	{
	FAST COUNT i, j;

	while (lv < uv)
		{
		for (i = lv - 1, j = uv; i < j; )
			{
			while (++i < j)
				if (0 < ordf(i, uv, &base))
					break;
			while (i < --j)
				if (ordf(j, uv, &base) < 0)
					break;
			if (i < j)
				excf(i, j, &base); /* segfault */
			}
		if (i != uv)
			excf(i, uv, &base);
		if (i - lv < uv - i)
			{
			_sort(lv, i - 1, ordf, excf, base);
			lv = i + 1;
			}
		else
			{
			_sort(i + 1, uv, ordf, excf, base);
			uv = i - 1;
			}
		}
	}

/*	external entry point
 */
VOID sort(n, ordf, excf, base)
	COUNT n;
	COUNT (*ordf)();
	VOID (*excf)();
	TEXT *base;
	{
	_sort(0, n - 1, ordf, excf, base);
	}

LOCAL TEXT *tmpName = "__TEMP__";
TEXT *ws_uname() {
    printf("uname: returning %s\n",tmpName);
    return tmpName;
}

UCOUNT lstou(UTINY *s) {
	return (s[1] << 8 | s[0]);
}

LONG lstol(UTINY *s) {
	return ((ULONG)(s[1] << 8 | s[0]) << 16 | (s[3] << 8 | s[2]));
}

TEXT *itols(TEXT *s, COUNT n) {
	s[0] = n;
	s[1] = n >> 8;
	return (s);
}

BYTES instr(TEXT *is, TEXT *p) {
	TEXT *s, *q;

	for (s = is; *s; s++)
		for (q = p; *q; q++)
			if (*s == *q) return (s - is);
	return (s - is);
}

TEXT *ltols(TEXT *pl, LONG lo) {
	COUNT *pi = (COUNT *)pl;

	itols((TEXT *)pi, (COUNT)(lo >> 16));
	itols((TEXT *)pi + 1, (COUNT)lo);
	return ((TEXT *)pi);
}

TEXT *pathnm(TEXT *buf, TEXT *n1, TEXT *n2) {
	COUNT i;
	TEXT *s, *t;

	s = n2 + lenstr(n2) - 1;

	t = "/";
	for (s = n1; s[i = instr(s, "/")]; s += i + 1)
                ;
	cpystr(buf, n2, t, s, NULL);
	return (buf);
}

BYTES ltob(TEXT *is, LONG ln, COUNT base) {
	FAST TEXT *s;
	LONG lb;
	s = is;
	if (ln < 0 && base <= 0) {
		ln = -ln;
		*s++ = '-';
	}
	if (base == 0) base = 10;
	else if (base < 0) base = -base;
	lb = base;
	if (ln < 0 || lb <= ln) s += ltob(s, (ULONG)ln / lb, base);
	*s = (ULONG)ln % lb + '0';
	if ('9' < *s) *s += ('a' - ('9' + 1));
	return (s - is + 1);
}

