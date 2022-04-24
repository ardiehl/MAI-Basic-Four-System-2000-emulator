/* dummy syscalls for MAI2000 w/o operating system */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <errno.h>
#include <errno.h>

//#include "sys/systraps.h"


/*
 only works when console is connected to scc port 0
*/

#define sccdata 0x600006
#define scccmd 0x600004

void writechar(int c) {
  char * data = (char *)sccdata;
  volatile char * cmd = (char *)scccmd;
  // wait for buffer empty
  while (!(*cmd & 0x04)) {};
  *data = (char) c;
  return;
}

#define SCC_A_CMD       0x600004
#define SCC_A_DATA      0x600006


char readchar(void) {
  volatile char * cmd = (char *)SCC_A_CMD;
  volatile char * data = (char *)SCC_A_DATA;

  // wait for char available
  while (!(*cmd & 0x01)) {};
  return(*data);
}

/* AD: we only support console
 * so return every char we have */
int read(int file, char *ptr, int len) {
  *ptr++ = readchar();
  return(1);
/*  
  int todo;
  printf("read,len=%d\n",len);
  for (todo = len; todo; --todo) {
    *ptr++ = readchar();
  }
  return(len); */
}

int lseek(int file, int ptr, int dir) {
  return 0;
}

int write(int file, char *ptr, int len) {
  int todo;

  for (todo = len; todo; --todo) {
    writechar(*ptr++);
  }
  return(len);
}

int close(int file) {
  return(-1);
}


// AD:


caddr_t sbrk(int incr) {
  register char *stack_ptr asm ("%sp");
  extern char _end;  /* Defined by the linker */
  static char *heap_end;
  char *prev_heap_end;

  if (heap_end == 0)
  {
    heap_end = &_end;
  }
  prev_heap_end = heap_end;
  if (heap_end + incr + 512 > stack_ptr)  // AD: add some spare here
    {
	  errno = ENOMEM;
	  return (void *) -1;
      //write (1, "Heap and stack collision\n", 25);
      //while(1);
    }
  heap_end += incr;
  return((caddr_t) prev_heap_end);
}

int isatty(int file) {
  return(1);
}

int fstat(int file, struct stat *st) {
  st->st_mode = S_IFCHR;
  return(0);
}

/*
int stat(char *filename, struct stat *st) {
  st->st_mode = S_IFCHR;
  return(0);
}*/

int open(const char *path, int flags) {
  return(0);
}


void _exit (int code) {
  asm ("trap #0x0f");  // drop to the internal debugger
  while (1);
}


int execve(char *name, char **argv, char **env) {
  errno = ENOMEM;
  return(-1);
}

int fork() {
  errno = EAGAIN;
  return(-1);
}

int getpid() {
  return(1);
}

int kill(int pid, int sig) {
  errno = EINVAL;
  return(-1);
}

int link(char *old, char *new) {
  errno = EMLINK;
  return(-1);
}

clock_t times(struct tms *buf) {
  return(-1);
}

int unlink(char *name) {
  errno = ENOENT;
  return(-1);
}

int wait(int *status) {
  errno = ECHILD;
  return(-1);
}

/* end of syscalls.c */
