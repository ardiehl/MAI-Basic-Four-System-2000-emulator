/***************************************************************************
 *  scc.c
 *
 *  Created: Nov, 22 2011
 *  Changed: Dec, 17 2011
 *  Armin Diehl <ad@ardiehl.de>
 ****************************************************************************
 * mai scc (serial boards on cmb) - Zilog z8530
 * see M8097C 'MAI 2000 Desktop Computer System Sevice Manual'
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "m68k.h"
#include "scc.h"
#include "sim.h"
#include "cmb.h"
#include "util.h"

#define SCC_NUM_REGS 16

/* s-record file for port B */
char * fileBuffer = NULL;
char * fileBufferPos;
int fileLen = 0;
int filePos = 0;

struct scc_t
  { int LastWrite;
	int wr[SCC_NUM_REGS];	/* WR0: Reg. pointers, various initialization commands */
	int rr[SCC_NUM_REGS];
	char recBuf;
  };

struct scc_t scc[2];

char  sccregs[][4] = {
	""};

int pollCnt = 0;

unsigned int scc_cmdRead (int port) {
	int idx;

	idx = scc[port].wr[0] & 0x07;
	if (((scc[port].wr[0] >> 3) & 0x07) == 1) idx+=8;
	scc[port].wr[0] = scc[port].wr[0] & 0xf0;	/* reset index to 0 */
	if (scc[port].rr[idx] != 0x04)
	    msgout (MSGC_INFO,MSG_SCC,MSG_READB,"Port%c cmd: %02x",port ? 'B' : 'A',scc[port].rr[idx]);
	if (idx == 0) {
		pollCnt++;
		if (pollCnt > 200) {
			usleep(50); /* for kbd poll */
			pollCnt = 0;
		}
        if (port == 1) {  /* check file for port B */
            if (filePos < fileLen) scc[port].wr[0] |= 1;
        }
	}
	return(scc[port].rr[idx]);
}


unsigned int scc_dataRead (int port) {
    char c;
    if (scc[port].rr[0] & 1) {
        scc[port].rr[0] &= 0xfe;
        msgout (MSGC_INFO,MSG_SCC,MSG_READB,"Port%c data: %02x",port ? 'B' : 'A',scc[port].recBuf);
        if (port == 0) return (unsigned int)scc[port].recBuf;
        else {
            if (scc[port].wr[14] & 0x10) {	/* are we in loopback mode ? */
                return (unsigned int)scc[port].recBuf;
            } else {
                c = *fileBufferPos; fileBufferPos++; filePos++;
                return c;
            }
        }
    } else {
		msgout (MSGC_INFO,MSG_SCC,MSG_READB,"Port%c data: no data available",port ? 'B' : 'A');
    }
    return 0;
}

unsigned int scc_read_byte(unsigned int address) {
	switch (address & SCC_REG_MASK) {
		case (SCC_A_CMD) : { return scc_cmdRead(0); break; }
		case (SCC_A_DATA): { return scc_dataRead(0); break; }
		case (SCC_B_CMD) : { return scc_cmdRead(1); break; }
		case (SCC_B_DATA): { return scc_dataRead(1); break; }
	}
	msgout (MSGC_ERR,MSG_SCC,MSG_READB,"%08x unknown address",address);
	return 0xff;
}

unsigned int scc_read_word(unsigned int address) {
	return scc_read_byte(address) | 0xff00;
}


void scc_cmdWrite (int port, int value) {
	int idx;

	msgout (MSGC_INFO,MSG_SCC,MSG_WRITEB,"Port%c cmd %02x",port ? 'B' : 'A',value);
	idx = scc[port].wr[0] & 0x07;
	if (((scc[port].wr[0] >> 3) & 0x07) == 1) idx+=8;
	scc[port].wr[0] = scc[0].wr[0] & 0xf0;	/* reset index to 0 */
	if (!((idx == 0) && (value == 0))) scc[port].wr[idx] = value & 0xff;
}


void scc_dataWrite (int port, int value) {

	msgout (MSGC_INFO,MSG_SCC,MSG_WRITEB,"Port%c data %02x",port ? 'B' : 'A',value);
	if (scc[port].wr[14] & 0x10) {	/* are we in loopback mode ? */
		/* boot rom echo's chars in loopbackmode and checks bit 7 in 7 bit mode */
		switch ((scc[port].wr[3] >> 6) & 0x03) {
			case 0: { value &= 0x1f; break; }	/* 5 bit */
			case 1: { value &= 0x7f; break; }	/* 7 bit */
			case 2: { value &= 0x3f; break; }	/* 6 bit */
		}
		scc[port].recBuf = (char)value & 0xff;
		scc[port].rr[0] |= 1;		/* Bit 0: rx char available */
	} else {
		if (port == 0) {	/* echo port A to console */
			if (!(cmb_getWriteRegs() & CMBWSTAT_INHSER)) {
				fputc(value & 0x7f,stderr);	/* stdout conflicts with keyboard input */
				fflush(stderr);
				fflush(stdout);
			}
		} else
        if (port == 1) {	/* echo port B to console */
			if (!(cmb_getWriteRegs() & CMBWSTAT_INHSER)) {
				fputc(value & 0x7f,stderr);	/* stdout conflicts with keyboard input */
				fflush(stderr);
				fflush(stdout);
			}
		}
	}
}


void scc_write_byte(unsigned int address, unsigned int value) {

	switch (address & SCC_REG_MASK) {
		case (SCC_A_CMD) : { scc_cmdWrite(0,value); return; }
		case (SCC_A_DATA): { scc_dataWrite(0,value); return; }
		case (SCC_B_CMD) : { scc_cmdWrite(1,value); return; }
		case (SCC_B_DATA): { scc_dataWrite(1,value); return; }
	}
	msgout (MSGC_ERR,MSG_SCC,MSG_WRITEB,"%02x to %08x",value,address);
}

void scc_write_word(unsigned int address, unsigned int value) {
	scc_write_byte(address,value & 0xff);
}


void scc_pollStatus(void) {
    char c;
    //static char lastC = 0;

    if (!(cmb_getWriteRegs() & CMBWSTAT_INHSER)) {  /* are the serial port drivers enabled ? */
        if (!(scc[0].wr[14] & 0x10)) {              /* are we not n loopback mode ? */
            if (kbhit()) {                          /* key available ? */
                c = getch();
                if (c == 24) { 	/* ^x */
                    setCtrlCPressed();              /* break execution (in sim.c) */
                } else {
                    scc[0].rr[0] |= 1;              /* char is available */
                    scc[0].recBuf = c;
				    /* TODO: interrupt handling */
                }
			}
        }
    }
}


struct scc_regDesc_t {
	char oneTxt[25];
	char nullTxt[25];
	void  (*proc)(int port, int regNum, int value);
};

void scc_printwr(int port, int regNum, int value) {
	int idx,i,j;
	char tx1[40],tx2[40];

	tx1[0]=0; tx2[0]=0;
	switch (regNum) {
		case 0: { 	idx = value & 0x07;
					if (((value >> 3) & 0x07) == 1) idx+=8;
					strcpy(tx1,"543: ");
					switch ((value >> 3) & 0x07) {
						case 0: { strcat(tx1,"null code"); break; }
						case 2: { strcat(tx1,"Reset Ext/Status Interrupts"); break; }
						case 3: { strcat(tx1,"Send Abort (SDLC)"); break; }
						case 4: { strcat(tx1,"Enable Int on Next Rx Character"); break; }
						case 5: { strcat(tx1,"Reset Tx Int Pending"); break; }
						case 6: { strcat(tx1,"Error Reset"); break; }
						case 7: { strcat(tx1,"Reset Highest IUS"); break; }
					}
					strcpy(tx2,"76: ");
					switch ((value >> 6) & 0x03) {
						case 0: { strcat(tx2,"null code"); break; }
						case 1: { strcat(tx2,"Reset Rx CRC Checker"); break; }
						case 2: { strcat(tx2,"Reset Tx CRC Generator"); break; }
						case 3: { strcat(tx2,"Reset Tx Underrun/EOM Latch"); break; }
					}
					printf("%s|%s|3210: idx %d|",tx1,tx2,idx);
					break;
		}
		case 3: { 	i = (value >> 6) & 3;
					switch (i) {
						case 0: { j = 5; break; }
						case 1: { j = 7; break; }
						case 2: { j = 6; break; }
						case 3: { j = 8; break; }
					}
					printf("|78:%d bit",j);
					break;
		}
		case 4: {	i = (value >> 2) & 3;
					switch (i) {
						case 0: { strcpy(tx1,"Sync Modes"); break; }
						case 1: { strcpy(tx1,"1 stop bit"); break; }
						case 2: { strcpy(tx1,"1,5 stop bits"); break; }
						case 3: { strcpy(tx1,"2 stop bits"); break; }
					}
					i = (value >> 4) & 3;
					switch (i) {
						case 0: { strcpy(tx2,"8 bit sync"); break; }
						case 1: { strcpy(tx2,"16 bit sync"); break; }
						case 2: { strcpy(tx2,"SDLC"); break; }
						case 3: { strcpy(tx2,"ext sync"); break; }
					}
					i = (value >> 6) & 3;
					switch (i) {
						case 0: { j=1; break; }
						case 1: { j=16; break; }
						case 2: { j=32; break; }
						case 3: { j=64; break; }
					}
					printf("|76:X%d-clock|54:%s|32:%s|",j,tx2,tx1);
					break;
		}
		case 5: {	i = (value >> 5) & 3;
					switch (i) {
						case 0: { j=5; break; }
						case 1: { j=7; break; }
						case 2: { j=6; break; }
						case 3: { j=8; break; }
					}
					printf("65:%d bits",j);
					break;
		}
		case 14: {	i = (value >> 5) & 7;
					switch (i) {
						case 0: { strcpy(tx1,"Null Command"); break; }
						case 1: { strcpy(tx1,"Enter Search Mode"); break; }
						case 2: { strcpy(tx1,"Reset Missing Clock"); break; }
						case 3: { strcpy(tx1,"Disable DPLL"); break; }
						case 4: { strcpy(tx1,"Set Source = BR Generator"); break; }
						case 5: { strcpy(tx1,"Set Source = /RTxC"); break; }
						case 6: { strcpy(tx1,"Set FM Mode"); break; }
						case 7: { strcpy(tx1,"Set NRZI Mode"); break; }
					}
					printf("765: %s",tx1);
		}
	}
}

struct scc_regDesc_t scc_regDesc[][8] = {
	{{ "","",scc_printwr },	/* W0:0 */
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "IntEna","IntDis",NULL },	/* W1:0 */
	 { "TxIntEna","TxIntDis",NULL },
	 { "ParIsSpecial","",NULL },
	 { "","",scc_printwr },
	 { "","",NULL },
	 { "DMA on","DMA off",NULL },
	 { "DMA req on","DMA req off",NULL },
	 { "DMA req ENA","DMA req DIS",NULL }},

	{{ "","",NULL },	/* W2:0 int vector */
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "RxENA","RxDIS",NULL },	/* W3:0 */
	 { "Sync Char Load Inhibit","",NULL },
	 { "Address Search Mode","",NULL },
	 { "Rx CRC Ena","Rx CRC Dis",NULL },
	 { "Enter Hunt Mode","",NULL },
	 { "Auto Enables","",NULL },
	 { "","",scc_printwr },
	 { "","",NULL }},

	{{ "Parity","noParity",NULL },	/* W4:0 */
	 { "even","odd",NULL },
	 { "","",scc_printwr },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "Tx CRC","Tx CRC off",NULL },	/* W5:0 */
	 { "RTS","rts",NULL },
	 { "CRC16","SDLC-CRC",NULL },
	 { "Tx ena","Tx dis",NULL },
	 { "Send Break on","Send Break off",NULL },
	 { "","",scc_printwr },
	 { "","",NULL },
	 { "DTR","dtr",NULL }},

	{{ "","",NULL },	/* W6:0 */
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "","",NULL },	/* W7:0 */
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "","",NULL },	/* W8:0 */
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "","",NULL },	/* W9:0 */
	 { "No Vector","Vector",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "","",NULL },	/* W10:0 */
	 { "Loop","no Loop",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "","",NULL },	/* W11:0 */
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "","",NULL },	/* W12:0 */
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "","",NULL },	/* W13:0 */
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "BRG Ena","BRG Dis",NULL },	/* W14:0 */
	 { "XTAL/RTxC","PCLK",NULL },
	 { "DTR auto","DTR man",NULL },
	 { "Auto Echo on","Auto Echo off",NULL },
	 { "Local Loopback on","Local Loopback off",NULL },
	 { "","",scc_printwr },
	 { "","",NULL },
	 { "","",NULL }},

	{{ "","",NULL },	/* W15:0 */
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL },
	 { "","",NULL }},
};

void printWr (int port, int regNum) {
	int i,j,value,bitval;

	printf("WR%d ",regNum);
	value = scc[port].wr[regNum];
	j = value;
	for (i=7; i>=0; i--) {
		bitval = ((j & 0x80) >> 7) & 1;
		j = j << 1;
		if (scc_regDesc[regNum][i].proc) {
			scc_regDesc[regNum][i].proc(port,regNum,value);
			printf("|");
		} else {
			if (bitval)
			  { if (scc_regDesc[regNum][i].oneTxt[0] != 0) printf("%d:1 %s|",i,scc_regDesc[regNum][i].oneTxt); } else
			  { if (scc_regDesc[regNum][i].nullTxt[0] != 0) printf("%d:0 %s|",i,scc_regDesc[regNum][i].nullTxt); }
		}
	}
	printf("\r\n");
}

void scc_pulse_reset(void) {
	memset(&scc,0,sizeof(scc));
	scc[0].rr[0] = 0x04;
	scc[1].rr[0] = 0x04;
}


/*char fileBuffer[32768];
int fileLen = 0;
int filePos = 0;
*/
void scc_file (int numArgs, struct args_t *args) {
    char fn[512];
    char s[10];
    int len,fileSize;
    FILE * f;

    fileLen = 0;
    strcpy(fn,args[0].txt);
	if (strrchr(fn,'.') == NULL) strcat(fn,".srec");

    if((f = fopen(fn, "rb")) == NULL) {
		if((f = fopen(fn, "rb")) == NULL) {
			printf ("unable to open %s\n",fn);
			return;
		}
	}
    fread(&s,1,4,f);
    s[4]=0;

    fseek (f, 0 , SEEK_END);
    fileSize = ftell (f);
    rewind (f);
    if (strcmp(s,"HDR1") == 0) {  /* diag file header */
        fseek(f,0x200,SEEK_CUR);
        fileSize -= 0x200;
    }
    fileBuffer = malloc (fileSize);
    len = fread(fileBuffer,1,fileSize,f);
    fclose(f);
    if (len != fileSize) {
        printf("can not load file, got %d bytes, expected %d\n",len,fileSize);
        free(fileBuffer);
        fileBuffer = NULL;
    } else {
        fileBufferPos = fileBuffer;
        fileLen = fileSize;
        filePos = 0;
        printf("%s loaded for scc port B\n",fn);
    }
}

void scc_showRegs(int numArgs, struct args_t *args) {
	int i;
	printf("WR  A  B  RR  A  B\r\n");
	printf("==================\r\n");
	for (i=0;i<SCC_NUM_REGS;i++) {
		printf("%2d %02x %02x  %2d %02x %02x\r\n",i,scc[0].wr[i],scc[1].wr[i],i,scc[0].rr[i],scc[1].rr[i]);
	}
	for (i=0;i<SCC_NUM_REGS;i++) {
	  printWr (0,i);
	}
}

void scc_help (int numArgs, struct args_t *args);

struct cmds_t sccCmds[] =
{
	{ "registers",	scc_showRegs, 0,0,0,"show scc registers"},
    { "file",	    scc_file    , 0,1,0,"load file for port B"},
	{ "?",			scc_help	, 0,0,0,"show this help"},
	{ "help",		scc_help	, 0,0,0,"show this help"},
    { "",  NULL, 0,0,0,""}
};

void scc_help (int numArgs, struct args_t *args) {
	showHelp ("scc help commands",sccCmds,0);
}

int scc_dbgCmd(int numArgs, struct args_t * args) {
	return findAndExecCommand (args[0].txt,sccCmds,numArgs-1,&args[1]);
}


int scc_save_state(FILE * f) {
    STATEWRITEVARS("scc");

    STATEWRITE(id,f);
    STATEWRITELEN(scc,f);
    return 1;
}

int scc_load_state(FILE * f) {
    STATEREADVARS("scc");

    STATEREADID(f);
    STATEREADLEN(scc,f);
    return 1;
}
