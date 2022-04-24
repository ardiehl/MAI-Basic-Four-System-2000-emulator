/***************************************************************************
 *            util.c
 *
 *  Tue November 22 21:23:11 2011
 *  Copyright  2011  Unknown
 *  <user@host>
 ****************************************************************************/
/*
 * util.c
 *
 * Copyright (C) 2011 - Unknown
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#ifndef eagle
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#endif

/*#include <stdlib.h>*/
#include <string.h>
#include <ctype.h>
#include "util.h"

#ifdef eagle

#define SCC_A_CMD  0x600004
#define SCC_A_DATA 0x600006

char getch(void)
{
  volatile char * cmd = (char *)SCC_A_CMD;
  volatile char * data = (char *)SCC_A_DATA;

  // wait for char available
  while (!(*cmd & 0x01)) {};
  return(*data);
}



int kbhit(void)
{
    volatile char * cmd = (char *)SCC_A_CMD;
    if (*cmd & 0x01) return 1;
    else return 0;
}

void kb_raw(void) {}
void kb_normal(void) {}

#else


int kbhit(void)
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

void kb_raw(void)
{
    struct termios ttystate;

    tcgetattr(STDIN_FILENO, &ttystate);
	ttystate.c_lflag &= ~ICANON;
	ttystate.c_lflag &= ~ECHO;
	ttystate.c_lflag &= ~ISIG;
	ttystate.c_cc[VMIN] = 1; /* minimum of number input read */
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}


void kb_normal(void)
{
	struct termios ttystate;

	tcgetattr(STDIN_FILENO, &ttystate);
	ttystate.c_lflag |= ICANON;
	ttystate.c_lflag |= ECHO;
	ttystate.c_lflag |= ISIG;
	tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}


char getch(void)
{
	kb_raw();
	char c = fgetc(stdin);
	if (c == 0x0a) c = 0x0d;
	kb_normal();
	return c;
}
#endif

/* Input string from console, input terminated by
 * any char in exitChars */
char inputString (char * s, int maxLen, const char * exitChars)
{
	char c;
	int i;
	*s = 0;
	
	do {
      c = getch();
	  i = strlen(s);

	  if ((c == KEY_BS) || (c == KEY_DEL)) {
		  if (i > 0) { 
			  i--; printf("%c %c",8,8);
			  s[i]=0;
		  }
	  } else
	  if (!(strchr(exitChars,c)) & (i < maxLen)) {
		  if (c >= ' ') {
		      s[i] = c; s[++i] = 0;
		      putchar(c);
		  }
	  }
	} while (!(strchr(exitChars,c)));
	switch (c) {
		case ' ': { putchar(' '); break; }
		case 13 : { printf("\r\n"); break; }
	}
    return c;
}

#define MAXLEN 64
char * readString (char * prompt) {
	char * s = malloc(MAXLEN+1);
	if (s) {
		printf(prompt);
		inputString (s,MAXLEN,"\r\n");
	}
	return s;
}

/* returns 1 on succsess */
int xtoui(const char* xs, unsigned int* result)
{
 size_t szlen = strlen(xs);
 int i, xv, fact;

 if (szlen > 0)
 {
  if (szlen>8) return 0; /* limut to 32 bit */

  *result = 0;
  fact = 1;

  for(i=szlen-1; i>=0 ;i--)
  {
   if (isxdigit(*(xs+i)))
   {
    if (*(xs+i)>='a') xv = ( *(xs+i) - 'a') + 10;
    else if ( *(xs+i) >= 'A') xv = (*(xs+i) - 'A') + 10;
    else xv = *(xs+i) - '0';
    *result += (xv * fact);
    fact *= 16;
   } else return 0;
  }
 } else return 0;
 return 1;
}


/* returns hex digit for lower 4 bits */
char hexNibble (unsigned int i)
{
	char digits[16] = "0123456789abcdef";
	return digits[i & 0x0f];
}

