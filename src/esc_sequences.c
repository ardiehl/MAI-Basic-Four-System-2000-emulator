/***************************************************************************
 *  esc_sequences.h
 *
 *  Created: Aug, 29 2026
 *  Changed:
 *  Armin Diehl <ad@ardiehl.de>
 ****************************************************************************
 * mai basic four system 2000 (eagle) emulator
 * translate of outgoing evdt sequences to vt100
 *

 */
#include "esc_sequences.h"
#include <string.h>
#include <stdio.h>

typedef struct bfescseqaux_t bfescseqaux_t;

struct bfescseqaux_t {
	char c;
	char *vtseq;
};

typedef struct bfescseq_t bfescseq_t;

struct bfescseq_t {
	int numArgsIn;		// number of chars after ESC
	int numArgsOut;		// number of variable args (decimal) to put into the vt sequence
	char * vtseq;		// vt100 sequence (with %d for the args)
	int offsetArgsOut;
	char secTabSize;	// second table e.g. for ESC g 0 to ESC g F
	bfescseqaux_t *secTab;
};



// for ESC g
struct bfescseqaux_t bfescseq_g_tab[] =
{
	{ '0', "\e[0m"	},						// mormal
	{ '1', "\e[0m\e[4m"	},					// b underline
	{ '2', "\e[0m\e[5m"	},					// b blink
	{ '3', "\e[0m\e[4\e[5m"	},				// b link underline
	{ '4', "\e[0m\e[7m"	},					// b reverse
	{ '5', "\e[0m\e[7m\e[4m"	},			// b reverse underline
	{ '6', "\e[0m\e[7m\e[5m"	},			// b reverse blink
	{ '7', "\e[0m\e[7m\e[4m\e[5m"	},		// b blink underline reverse
	{ '8', "\e[0m\e[2m"	},					// dark normal
	{ '9', "\e[0m\e[2m\e[4m"	},			// dark underline
	{ 'A', "\e[0m\e[2m\e[5m"	},			// dark blink
	{ 'B', "\e[0m\e[2m\e[4m\e[5m"	},		// d_blink_underline
	{ 'C', "\e[0m\e[2m\e[7m"	},			// d_reverse
	{ 'D', "\e[0m\e[2m\e[7m\e[4m"	},		// d_reverse_underline
	{ 'E', "\e[0m\e[2m\e[7m\e[5m"	},		// d_reverse_blink
	{ 'F', "\e[0m\e[2m\e[7m\e[5m\e[4m"	}	// d_reverse_blink underline

};

#define BFESCSEQ_TAB_FIRST 32
#define BFESCSEQ_TAB_LAST 127

struct bfescseq_t bfescseq_tab[] =
{
	{ 0, 0, NULL }, // 0x20 (32) - ' '
	{ 0, 0, NULL }, // 0x21 (33) - '!'
	{ 1, 0, NULL }, // 0x22 (34) - '"' unlock_keyboard
	{ 1, 0, NULL }, // 0x23 (35) - '#' lock_keyboard
	{ 0, 0, NULL }, // 0x24 (36) - '$'
	{ 0, 0, NULL }, // 0x25 (37) - '%'
	{ 1, 0, NULL }, // 0x26 (38) - '&' clear_foreground 1 (start protected)
	{ 1, 0, NULL }, // 0x27 (39) - ''' clear_foreground 3
	{ 0, 0, NULL }, // 0x28 (40) - '('
	{ 0, 0, NULL }, // 0x29 (41) - ')'
	{ 1, 0, "\e[2J\e[H" }, // 0x2A (42) - '*' clear screen
	{ 1, 0, NULL }, // 0x2B (43) - '+' clear_foreground 2
	{ 0, 0, NULL }, // 0x2C (44) - ','
	{ 0, 0, NULL }, // 0x2D (45) - '-'
	{ 0, 0, NULL }, // 0x2E (46) - '.'
	{ 0, 0, NULL }, // 0x2F (47) - '/'
	{ 0, 0, NULL }, // 0x30 (48) - '0'
	{ 0, 0, NULL }, // 0x31 (49) - '1'
	{ 0, 0, NULL }, // 0x32 (50) - '2'
	{ 0, 0, NULL }, // 0x33 (51) - '3'
	{ 1, 0, NULL }, // 0x34 (52) - '4' trnsmt_line
	{ 1, 0, NULL }, // 0x35 (53) - '5' transmit_screen
	{ 1, 0, NULL }, // 0x36 (54) - '6' trnsmt_line_protected
	{ 1, 0, NULL }, // 0x37 (55) - '7' trnsmt_scr_protected
	{ 0, 0, NULL }, // 0x38 (56) - '8'
	{ 0, 0, NULL }, // 0x39 (57) - '9'
	{ 0, 0, NULL }, // 0x3A (58) - ':'
	{ 0, 0, NULL }, // 0x3B (59) - ';'
	{ 0, 0, NULL }, // 0x3C (60) - '<'
	{ 3, 2, "\e[%d;%dH", -31 }, // 0x3D (61) - '=' xy
	{ 0, 0, NULL }, // 0x3E (62) - '>'
	{ 1, 0, NULL }, // 0x3F (63) - '?' read_cursor
	{ 0, 0, NULL }, // 0x40 (64) - '@'
	{ 1, 0, NULL }, // 0x41 (65) - 'A' begin_bypass
	{ 1, 0, NULL }, // 0x42 (66) - 'B' end_bypass
	{ 0, 0, NULL }, // 0x43 (67) - 'C'
	{ 0, 0, NULL }, // 0x44 (68) - 'D'
	{ 1, 0, "\e[L" }, // 0x45 (69) - 'E' insert line
	{ 2, 0, NULL }, // 0x46 (70) - 'F' expanded_print
	{ 0, 0, NULL }, // 0x47 (71) - 'G'
	{ 0, 0, NULL }, // 0x48 (72) - 'H'
	{ 0, 0, NULL }, // 0x49 (73) - 'I'
	{ 0, 0, NULL }, // 0x4A (74) - 'J'
	{ 0, 0, NULL }, // 0x4B (75) - 'K'
	{ 0, 0, NULL }, // 0x4C (76) - 'L'
	{ 0, 0, NULL }, // 0x4D (77) - 'M'
	{ 0, 0, NULL }, // 0x4E (78) - 'N'
	{ 0, 0, NULL }, // 0x4F (79) - 'O'
	{ 0, 0, NULL }, // 0x50 (80) - 'P' print_screen
	{ 1, 0, "\e[@" }, // 0x51 (81) - 'Q' ic
	{ 1, 0, "\e[M" }, // 0x52 (82) - 'R' dl
	{ 0, 0, NULL }, // 0x53 (83) - 'S'
	{ 1, 0, "\e[K" }, // 0x54 (84) - 'T' eol
	{ 0, 0, NULL }, // 0x55 (85) - 'U'
	{ 0, 0, NULL }, // 0x56 (86) - 'V'
	{ 1, 0, "\e[P" }, // 0x57 (87) - 'W' delete character
	{ 0, 0, NULL }, // 0x58 (88) - 'X'
	{ 1, 0, "\e[J" }, // 0x59 (89) - 'Y' clear to eos
	{ 0, 0, NULL }, // 0x5A (90) - 'Z'
	{ 1, 0, "\e[H" }, // 0x5B (91) - '[' home
	{ 0, 0, NULL }, // 0x5C (92) - '\'
	{ 0, 0, NULL }, // 0x5D (93) - ']'
	{ 0, 0, NULL }, // 0x5E (94) - '^'
	{ 0, 0, NULL }, // 0x5F (95) - '_' page_mode
	{ 0, 0, NULL }, // 0x60 (96) - '`'
	{ 0, 0, NULL }, // 0x61 (97) - 'a'
	{ 0, 0, NULL }, // 0x62 (98) - 'b'
	{ 0, 0, NULL }, // 0x63 (99) - 'c'
	{ 0, 0, NULL }, // 0x64 (100) - 'd'
	{ 0, 0, NULL }, // 0x65 (101) - 'e'
	{ 0, 0, NULL }, // 0x66 (102) - 'f'
	{ 2, 0, NULL, 0, 16, bfescseq_g_tab }, // 0x67 (103) - 'g' b_normal
	{ 0, 0, NULL }, // 0x68 (104) - 'h'
	{ 0, 0, NULL }, // 0x69 (105) - 'i'
	{ 0, 0, NULL }, // 0x6A (106) - 'j'
	{ 0, 0, NULL }, // 0x6B (107) - 'k'
	{ 0, 0, NULL }, // 0x6C (108) - 'l'
	{ 0, 0, NULL }, // 0x6D (109) - 'm'
	{ 0, 0, NULL }, // 0x6E (110) - 'n'
	{ 0, 0, NULL }, // 0x6F (111) - 'o'
	{ 0, 0, NULL }, // 0x70 (112) - 'p'
	{ 0, 0, NULL }, // 0x71 (113) - 'q'
	{ 0, 0, NULL }, // 0x72 (114) - 'r'
	{ 0, 0, NULL }, // 0x73 (115) - 's'
	{ 0, 0, NULL }, // 0x74 (116) - 't'
	{ 0, 0, NULL }, // 0x75 (117) - 'u'
	{ 0, 0, NULL }, // 0x76 (118) - 'v'
	{ 0, 0, NULL }, // 0x77 (119) - 'w'
	{ 0, 0, NULL }, // 0x78 (120) - 'x'
	{ 0, 0, NULL }, // 0x79 (121) - 'y'
	{ 0, 0, NULL }, // 0x7A (122) - 'z'
	{ 0, 0, NULL }, // 0x7B (123) - '{'
	{ 0, 0, NULL }, // 0x7C (124) - '|'
	{ 0, 0, NULL }, // 0x7D (125) - '}'
	{ 0, 0, NULL }, // 0x7E (126) - '~'
	{ 0, 0, NULL }, // 0x7F (127)

};



 void addCharToOutBuf (char c, char * outBuf, int outBufSize) {
 	int len = 0;

    while (*outBuf++) len++;
    if (len >= outBufSize-1) return;

    *(outBuf - 1) = c;
    *outBuf = '\0';
 }

 void addStrToOutBuf (char * s, char * outBuf, int outBufSize) {
 	if (s == NULL) return;
 	int outBufLen = strlen(outBuf);
 	int sLen = strlen(s);
 	int space = outBufSize - outBufLen -1;
 	if (sLen > space) sLen = space;
 	strncat(outBuf, s, sLen);
 }


 void bfseq_init (bfseq_State_t * sta) {
 	sta->inSequence = 0;
 }

 // returns number of characters out
 // data will be placed in outBuf
 int bfseq_processChar (bfseq_State_t * sta, char c, char * outBuf, int outBufSize) {
 	char tmpBuf[255];
 	char * vtseq;

 	*outBuf = 0;
	if (! sta->inSequence) {
		// TODO: one character code translation
		if (c == 27) {
			sta->inSequence = 1;
			sta->seqBufLen = 0;
			return 0;
		}
		addCharToOutBuf(c,outBuf,outBufSize);
		return 1;
	}
	if (sta->seqBufLen == 0) {		// first char after ESC
		//sta->seqBuf[0] = c; sta->seqBufLen = 1; sta->seqBuf[sta->seqBufLen] = 0;
		if (c < BFESCSEQ_TAB_FIRST || c > BFESCSEQ_TAB_LAST) {
			// we do not have anything for that sequence, send it as is
			addCharToOutBuf('\e',outBuf, outBufSize);
			addCharToOutBuf(c,outBuf, outBufSize);
			sta->inSequence = 0;
			return 2;
		}
	}

	// add it to the buffer
	if (sta->seqBufLen < BFESC_MAXLEN-1) {
		sta->seqBuf[sta->seqBufLen] = c;
		sta->seqBufLen++;
		sta->seqBuf[sta->seqBufLen] = 0;

		if (sta->seqBufLen >= bfescseq_tab[sta->seqBuf[0] - BFESCSEQ_TAB_FIRST].numArgsIn) {	// got the complete sequence ?
			vtseq = bfescseq_tab[sta->seqBuf[0] - BFESCSEQ_TAB_FIRST].vtseq;
			switch (bfescseq_tab[sta->seqBuf[0] - BFESCSEQ_TAB_FIRST].numArgsOut) {
			case 0:
				// check if we have a seconds tab
				if (bfescseq_tab[sta->seqBuf[0] - BFESCSEQ_TAB_FIRST].secTab) {
					c = sta->seqBuf[sta->seqBufLen-1];
					for (int i=0; i < bfescseq_tab[sta->seqBuf[0] - BFESCSEQ_TAB_FIRST].secTabSize; i++) {	// look up last char in aux table
						if (bfescseq_tab[sta->seqBuf[0] - BFESCSEQ_TAB_FIRST].secTab[i].c == c) {
							vtseq = bfescseq_tab[sta->seqBuf[0] - BFESCSEQ_TAB_FIRST].secTab[i].vtseq;
							addStrToOutBuf(vtseq,outBuf,outBufSize);
							sta->inSequence = 0;
							if (vtseq) return strlen(vtseq);
							return 0;
						}
					}
					// not found
					sta->inSequence = 0;
					return 0;
				}

				addStrToOutBuf(vtseq,outBuf,outBufSize);
				sta->inSequence = 0;
				if (vtseq != NULL)
					return (strlen(vtseq));
				else
					return 0;
				break;
			case 1:
				if (vtseq) {
					int offset = bfescseq_tab[sta->seqBuf[0] - BFESCSEQ_TAB_FIRST].offsetArgsOut;
					snprintf(tmpBuf,sizeof(tmpBuf)-1,vtseq,((int)sta->seqBuf[1])+offset);
					addStrToOutBuf(tmpBuf,outBuf,outBufSize);
				}
				sta->inSequence = 0;
				return (strlen(tmpBuf));
				break;
			case 2:
				if (vtseq) {
					int offset = bfescseq_tab[sta->seqBuf[0] - BFESCSEQ_TAB_FIRST].offsetArgsOut;
					snprintf(tmpBuf,sizeof(tmpBuf)-1,vtseq,((int)sta->seqBuf[1])+offset,((int)sta->seqBuf[2])+offset);
					addStrToOutBuf(tmpBuf,outBuf,outBufSize);
				}
				sta->inSequence = 0;
				return (strlen(tmpBuf));
				break;
			}

			// unsupported number of args
			sta->inSequence = 0;
			return 0;

		} else {
			// wait for the next character
			return 0;
		}
	}

	// should never happen, we reached buffer len
	addCharToOutBuf('\e',outBuf,outBufSize);
	addStrToOutBuf(sta->seqBuf,outBuf,outBufSize);
	sta->inSequence = 0;
	return sta->seqBufLen+1;
 }
