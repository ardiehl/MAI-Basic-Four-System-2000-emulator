/* routines for Whitesmiths c needed by as/link/obj/rel/lord that are
   not included in a standard c library
 * AD 10/2026
 */

#ifndef __ws_libc_h
#define __ws_libc_h

#include "std.h"
#include <stdio.h>

/* needed for ld, needs to be undefined in ws_libc.c */
#define fread(fd,buf,count) read(fd,buf,count)
#define fwrite(fd,buf,count) write(fd,buf,count)

/* we can not use string.h */
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
long unsigned int strlen(const char *s);

void free(void *ptr);

/* not defined by gcc with -std=c89 */
extern FILE *fdopen (int fd, const char *modes);

TEXT *alloc (BYTES nBytes, void *link);
TEXT *buybuf(TEXT *s, BYTES n); /* Alloc and copy in */


TEXT *wsfree(TEXT *pcell, TEXT *link);

BYTES lenstr (TEXT * s);
BOOL cmpbuf (TEXT * s1, TEXT * s2, BYTES n);
BYTES scnstr (TEXT *s, TEXT c);
/* is s2 a prefix of s1 */
BOOL prefix(TEXT *s1, TEXT *s2);

int create (TEXT * fname, COUNT mode, BYTES rsize);
BYTES cpybuf (TEXT *s1,TEXT *s2, BYTES n);
void putstr (int fd, ...);
TEXT * cpystr (TEXT *target, ...);

BYTES btol (TEXT *s, BYTES n, LONG *l, COUNT base);
/*	collect flags from command argument list
 */
TEXT *getflags(BYTES *pac, TEXT ***pav, TEXT *fmt, ... );
int getfiles (int *pac, TEXT ***pav, int dfd, int efd);

/* build ac and av list from STDIN */


BYTES putl (FILE *pfio, TEXT *s, BYTES n);

BOOL cmpstr (TEXT *s1, TEXT * s2);

BYTES itob (TEXT *is, ARGINT n, COUNT base);

typedef void (*exit_func_t)(void);
typedef exit_func_t (*onexit_t)(exit_func_t);

exit_func_t onexit(exit_func_t new_func);
exit_func_t onintr(exit_func_t new_func);

/* gtc was a macro using FIO internals */
COUNT gtc (FILE *f);

COUNT lstoi(UTINY *s);

int getbfiles(ARGINT *pac, TEXT ***pav, int dfd, int efd, BYTES rsize);

/* remark - print non-fatal error message */
VOID remark(TEXT *s1, TEXT *s2);

/* print error message and exit */
VOID error(TEXT *s1, TEXT *s2);

/* get a text line from stdin*/
BYTES getlin(TEXT *s, BYTES n);

#define uname ws_uname
TEXT *ws_uname();

UCOUNT lstou(UTINY *s);

LONG lstol(UTINY *s);

TEXT *itols(TEXT *s, COUNT n);

BYTES instr(TEXT *is, TEXT *p);

TEXT *ltols(TEXT *pl, LONG lo);

TEXT *pathnm(TEXT *buf, TEXT *n1, TEXT *n2);

BYTES ltob(TEXT *is, LONG ln, COUNT base);

#endif
