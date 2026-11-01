/*	THE STANDARD HEADER
 *	copyright (c) 1978 by Whitesmiths, Ltd.

 AD: Taken from the feely available Witheshmis runtime library available at
 https://github.com/hansake/Whitesmiths-C-compiler

 Therefore non-commerecial purposes only

 This version is for compiling with modern gcc and not for the Whitesmiths compiler
 */

#ifndef __std_h
#define __std_h

#ifndef USESTDCLIB
#define USESTDCLIB
#endif

#ifdef USESTDCLIB
 #include <stdio.h>
 #include <unistd.h>
 #include <stdint.h>
#endif

/* the pseudo storage classes
 */
#define FAST	register
#define GLOBAL	extern
#define IMPORT	extern
#define INTERN	static
#define LOCAL	static

/* the pseudo types
 */
#ifdef UTEXT
typedef unsigned char TEXT;
#else
typedef char TEXT;
#endif
typedef TEXT TBOOL;
typedef int8_t TINY;
typedef double DOUBLE;
typedef int32_t ARGINT, BOOL, VOID;
typedef int32_t LONG;
#ifndef USESTDCLIB
typedef int16_t COUNT, FILE, METACH;
#else
typedef short COUNT, METACH;
#endif
typedef uint32_t BYTES;
typedef unsigned char UTINY;
typedef uint32_t ULONG;
typedef uint16_t BITS, UCOUNT;

/* system parameters
 */
#define STDIN	0
#define STDOUT	1
#define STDERR	2
#define YES		1
#define NO		0
/* AD: */
#ifndef __STDC__
#define NULL	0
#endif
#define FOREVER	for (;;)
#define BUFSIZE	512
#define BWRITE	-1
#define READ	0
#define WRITE	1
#define UPDATE	2
#ifdef EOF
#undef EOF
#endif
#define EOF		-1
#define BYTMASK	0377

/*	macros
 */
#define abs(x)		((x) < 0 ? -(x) : (x))
#ifndef USESTDCLIB
#define gtc(pf)	(0 < (pf)->_nleft ? (--(pf)->_nleft, \
		*(pf)->_pnext++ & BYTMASK) : getc(pf))
#endif
#define isalpha(c)	(islower(c) || isupper(c))
#define isdigit(c)	('0' <= (c) && (c) <= '9')
#define islower(c)	('a' <= (c) && (c) <= 'z')
#define isupper(c)	('A' <= (c) && (c) <= 'Z')
#define iswhite(c)	((c) <= ' ' || 0177 <= (c))
#define max(x, y)	(((x) < (y)) ? (y) : (x))
#define min(x, y)	(((x) < (y)) ? (x) : (y))
#define ptc(pf, c)	(((pf)->_nleft < 512) ? (pf)->_buf[(pf)->_nleft++] = (c) :\
	putc(pf, c))
#define tolower(c)	(isupper(c) ? ((c) + ('a' - 'A')) : (c))
#define toupper(c)	(islower(c) ? ((c) - ('a' - 'A')) : (c))

/* the file IO structure
 */
#ifndef USESTDCLIB
typedef struct fio
	{
	FILE _fd;
	COUNT _nleft;
	COUNT _fmode;
	TEXT *_pnext;
	TEXT _buf[BUFSIZE];
	} FIO;
#endif

#ifdef USESTDCLIB
#include <ws_libc.h>
#endif

#endif
