/***************************************************************************
 *  util.h
 *
 *  Tue November 22 21:23:11 2011
 *  Copyright  2011 Armin Diehl
 *  <ad@ardiehl.de>
 ****************************************************************************/
/*
 * util.h
 *
 * Copyright (C) 2011 - Armin Diehl
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

#ifndef UTIL_H
#define UTIL_H


#define KEY_ESC 27
#define KEY_ENTER 13
#define KEY_BS 8
#define KEY_DEL 0x7f

/* convert hex str to unsigned int, max 32 bit
   returns 1 on succsess */
int xtoui(const char* xs, unsigned int* result);


void kb_normal(void);
void kb_raw(void);

/* returns 1 if console key is available */
int kbhit(void);

/* return one character from console without echo */
char getch(void);

/* Input string from console, input terminated by
 * any char in exitChars */
char inputString (char * s, int maxLen, const char * exitChars);

/* free is needed for result */
char * readString (char * prompt);

/* returns hex digit for lower 4 bits */
char hexNibble (unsigned int i);

#endif
