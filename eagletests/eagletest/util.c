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
/*#include <stdlib.h>*/
#include <string.h>
#include <ctype.h>
#include "util.h"

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

// for vbcc
// _lmodu.__lmods.__ldivu.__ldivs.__mods.__modu.__divs.__divu.


