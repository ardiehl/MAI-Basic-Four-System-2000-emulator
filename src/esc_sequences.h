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
#ifndef ESC_SEQUENCES_H
#define ESC_SEQUENCES_H

#include <stddef.h>

/* EVDT squences as defined in /etc/ttymntbl/evdt.mntbl

home                    	1B	5B	48
clear                   	1B	2A
eos                     	1B	59
eol                     	1B	54
left                    	08
right                   	0C
up                      	0B
down                    	0A
ic                      	1B	51
dc                      	1B	57
il                      	1B	45
dl                      	1B	52
b_normal                	1B	67	30
b_underline             	1B	67	31
b_blink                 	1B	67	32
b_blink_underline       	1B	67	33
b_reverse               	1B	67	34
b_reverse_underline     	1B	67	35
b_reverse_blink         	1B	67	36
b_all                   	1B	67	37
d_normal                	1B	67	38
d_underline             	1B	67	39
d_blink                 	1B	67	41
d_blink_underline       	1B	67	42
d_reverse               	1B	67	43
d_reverse_underline     	1B	67	44
d_reverse_blink         	1B	67	45
d_all                   	1B	67	46
xy                      	1B	3D
read_cursor             	1B	3F
clear_foreground        	1B	26	1B	2B	1B	27
expanded_print          	1B	46	33
start_protect           	1B	26
end_protect             	1B	27
transmit_screen         	1B	35
page_mode               	1B	5F
print_screen            	1B	50
begin_bypass            	1B	41
end_bypass              	1B	42
lock_keyboard           	1B	23
unlock_keyboard         	1B	22
trnsmt_scr_protected    	1B	37
trnsmt_line             	1B	34
trnsmt_line_protected   	1B	36
*/

#define BFESC_MAXLEN 5

 typedef struct bfseq_State_t bfseq_State_t;
 struct bfseq_State_t {
 	int inSequence;
 	char seqBuf[BFESC_MAXLEN];
 	int seqBufLen;
};

void addCharToOutBuf (char c, char * outBuf, int outBufSize);
void addStrToOutBuf (char * s, char * outBuf, int outBufSize);

// initializes the struct, no alloc
void bfseq_init (bfseq_State_t * sta);

 // returns number of characters out
 // data will be placed in outBuf
 // outBufSize is max size of outBuf
int bfseq_processChar (bfseq_State_t * sta, char c, char * outBuf, int outBufSize);

#endif
