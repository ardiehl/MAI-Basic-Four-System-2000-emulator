// GCC test for MAI 2000

#ifdef EAGLE

#include <stdint.h>

#define R_SCCCMD  0x600004
#define R_SCCDATA 0x600006

//#define SCCCMD  ((uint8_t *) R_SCCCMD)
#define SCCDATA  ((char *) R_SCCDATA)

void outbyte (char c) {
	volatile uint8_t * SCCCMD = (uint8_t *) R_SCCCMD;
	do {
	} while ((*SCCCMD & 0x04) == 0);  // scc status register, bit 2=1 -> tx buffer empty
	*SCCDATA = c;
	if (c == 10) outbyte(13);       // for printf("\n")
}

void outstr (char *c) {
	while (*c) {
		outbyte(*c);
 		c++;
	}
}


void _exit (int rc)
{
	asm ("trap #0x0f");  // drop to the internal debugger
  	while (1);
}

char inbyte (void) {

        volatile uint8_t * SCCCMD = (uint8_t *) R_SCCCMD;

        do {
        } while ((*SCCCMD & 0x01) == 0);
	//if (*SCCDATA == 26) _exit(0);  // ^Z for debugger
        return *SCCDATA & 0x7f;
}


//#include <sys/config.h>
#include <_syslist.h>

int write (int file, char *ptr, int len) {
	int i=len;
	while(i) {
		outbyte(*ptr);
		if(*ptr == 10) outbyte(13);
		ptr++; i--;
	}
	return len;
}


/* Version of sbrk for no operating system.  */

void *
sbrk (incr)
     int incr;
{
   extern char   _end;				/* Set by linker, end of code  */
   extern char   _heapmax;			/* Set by linker, end of heap, see linker script  */
   static char * heap_end;			/* current end of heap */
   char *        prev_heap_end;

   if (heap_end == 0)
     heap_end = & _end;

   prev_heap_end = heap_end;
   heap_end += incr;
   if (heap_end > &_heapmax) {		/* avoid stack corruption when heap overrides stack */
	write(1,"\rHeap overflow in eagle.c, sbrk\r\n",34);
	return ((void *)-1);

	//_exit(1);
   }

   return (void *) prev_heap_end;
}




#define ENOSYS 38
//#include "warning.h"
#define RETE { errno=ENOSYS; return -1; }

extern int errno;

int open (char *file, int flags, int mode) RETE
int close (int fildes) RETE

int read (int file, char *ptr, int len) {
	if (file == 0) {
		*ptr = inbyte();
		if (*ptr == 26) _exit(0);
		return 1;
	} else {
		errno = 9; //EBADF;
		return -1;
	}
}

int lseek (int file, int ptr, int dir) RETE
int kill (int pid, int sig) RETE
int getpid (void) RETE
int fstat (int fildes, void *st) RETE
int isatty (int file) RETE



#endif // EAGLE
