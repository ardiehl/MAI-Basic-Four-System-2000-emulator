/*
 * BOSS/IX low level interface for the Green Hills C library, replacing the
 * generic os.c of the tape. System calls go through _bx(call, d0, d1, d2),
 * see crt0.s. C file numbers 0, 1 and 2 map to BOSS/IX channels 1, 2 and 2.
 * Files open read only with call 0x66 and read/write with the native call
 * 0x8b; creat() makes a string file with trap #0 call 0x89 (_bxcreate in
 * crt0.s), the way Business BASIC STRING does. lseek uses 0x0a (absolute
 * position), 0x10 (current position) and the size from 0x2f. sbrk moves
 * the memory limit with 0x12 (get) and 0x11 (set); time comes from 0x36,
 * seconds since 1 January 1980.
 */
#include <stdio.h>
#include <errno.h>
#include <types.h>
#include <stat.h>
#include <time.h>

#define BX_READ		0x0d
#define BX_WRITE	0x0e
#define BX_EXIT		0x29
#define BX_OPEN		0x66
#define BX_CLOSE	0x05
#define BX_NOPEN	0x8b
#define BX_UNLINK	0x17
#define BX_EXISTS	(-37)
#define BX_SEEK		0x0a
#define BX_TELL		0x10
#define BX_FSTAT	0x2f
#define BX_SETMEM	0x11
#define BX_GETMEM	0x12
#define BX_TIME		0x36
#define EPOCH1980	315532800L

static char isopen[_NFILE];
static int chan[_NFILE];

Init_IO()
{
    isopen[0] = isopen[1] = isopen[2] = 1;
    chan[0] = 1;
    chan[1] = 2;
    chan[2] = 2;
}

static find_free()
{
    int fno;

    for (fno = 3; fno < _NFILE; fno++)
	if (!isopen[fno])
	    return(fno);
    errno = EMFILE;
    return(-1);
}

open(filename, mode)
char *filename;
{
    int fno, c;

    if ((fno = find_free()) == -1)
	return(-1);
    if ((mode & 3) == 0)
	c = _bx(BX_OPEN, filename, 0, 0);
    else
	c = _bx(BX_NOPEN, filename, 0, 0);
    if (c < 0) {
	errno = ENOENT;
	return(-1);
    }
    isopen[fno] = 1;
    chan[fno] = c;
    return(fno);
}

creat(filename, prot)
char *filename;
{
    int r;

    r = _bxcreate(0, filename, 0, 0, 1, 0, 0, 0);
    if (r == BX_EXISTS) {
	_bx(BX_UNLINK, filename, 0, 0);
	r = _bxcreate(0, filename, 0, 0, 1, 0, 0, 0);
    }
    if (r < 0) {
	errno = EACCES;
	return(-1);
    }
    return(open(filename, 1));
}

close(fno)
{
    if (fno < 0 || fno >= _NFILE || !isopen[fno]) {
	errno = EBADF;
	return(-1);
    }
    isopen[fno] = 0;
    if (fno >= 3)
	_bx(BX_CLOSE, chan[fno], 0, 0);
    return(0);
}

read(fno, buf, size)
char *buf;
{
    int n;

    if (fno < 0 || fno >= _NFILE || !isopen[fno]) {
	errno = EBADF;
	return(-1);
    }
    n = _bx(BX_READ, chan[fno], buf, size);
    if (n < 0) {
	errno = EIO;
	return(-1);
    }
    return(n);
}

write(fno, buf, size)
char *buf;
{
    int n;

    if (fno < 0 || fno >= _NFILE || !isopen[fno]) {
	errno = EBADF;
	return(-1);
    }
    n = _bx(BX_WRITE, chan[fno], buf, size);
    if (n < 0) {
	errno = EIO;
	return(-1);
    }
    return(n);
}

lseek(fno, offset, end)
long offset;
{
    long pos, st[24];

    if (fno < 0 || fno >= _NFILE || !isopen[fno]) {
	errno = EBADF;
	return(-1);
    }
    if (fno < 3) {
	errno = ESPIPE;
	return(-1);
    }
    switch (end) {
    case 0:
	pos = offset;
	break;
    case 1:
	pos = _bx(BX_TELL, chan[fno], 0, 0) + offset;
	break;
    case 2:
	if (_bx(BX_FSTAT, chan[fno], st, 0) < 0) {
	    errno = EIO;
	    return(-1);
	}
	pos = *(long *)((char *)st + 14) + offset;
	break;
    default:
	errno = EINVAL;
	return(-1);
    }
    if (pos < 0 || _bx(BX_SEEK, chan[fno], pos, 0) < 0) {
	errno = EINVAL;
	return(-1);
    }
    return(pos);
}

access(name, prot)
char *name;
{
    return(-1);
}

unlink(name)
char *name;
{
    if (_bx(BX_UNLINK, name, 0, 0) < 0) {
	errno = ENOENT;
	return(-1);
    }
    return(0);
}

fstat(fno, statptr)
struct stat *statptr;
{
    statptr->st_dev = 0;
    statptr->st_ino = fno;
    return(0);
}

stat(name, statptr)
char *name;
struct stat *statptr;
{
    return(-1);
}

isatty(fno)
{
    return(fno >= 0 && fno < 3);
}

char *sbrk(size)
{
    long cur, top;

    cur = _bx(BX_GETMEM, 0, 0, 0);
    if (cur & 1)
	cur++;
    top = cur + size;
    if (top & 1)
	top++;
    if (size != 0 && _bx(BX_SETMEM, top, 0, 0) < 0) {
	errno = ENOMEM;
	return((char *)-1);
    }
    return((char *)cur);
}

getpid()
{
    return(17);
}

_exit(code)
{
    _bx(BX_EXIT, code, 0, 0);
}

gettimeofday(timevalp, timezonep)
struct timeval *timevalp;
struct timezone *timezonep;
{
    timevalp->tv_sec = time((long *)0);
    timevalp->tv_usec = 0;
    return(0);
}

long time(t)
long *t;
{
    long s;

    s = _bx(BX_TIME, 0, 0, 0) + EPOCH1980;
    if (t)
	*t = s;
    return(s);
}

struct tm *localtime(seconds)
long *seconds;
{
    static struct tm tm;
    static int mdays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    long t, n;

    t = *seconds;
    tm.tm_sec = t % 60;
    tm.tm_min = (t /= 60) % 60;
    tm.tm_hour = (t /= 60) % 24;
    t /= 24;
    for (tm.tm_year = 70; t >= (n = tm.tm_year % 4 ? 365 : 366); tm.tm_year++)
	t -= n;
    mdays[1] = tm.tm_year % 4 ? 28 : 29;
    for (tm.tm_mon = 0; t >= mdays[tm.tm_mon]; tm.tm_mon++)
	t -= mdays[tm.tm_mon];
    tm.tm_mday = t + 1;
    return(&tm);
}

struct tm *gmtime(seconds)
long *seconds;
{
    return(localtime(seconds));
}

/* The kernel starts a program with 0, argc, argv and envp on the stack;
 * crt0.s keeps that stack pointer in _sp0. */
extern long *_sp0;

char **args(argc, envp)
int *argc;
char ***envp;
{
    *argc = _sp0[1];
    *envp = (char **)_sp0[3];
    return((char **)_sp0[2]);
}
