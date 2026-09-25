/***************************************************************************
 * fd.c
 *
 *  Created: Dec, 12 2011
 *  Changed: May, 06 2015
 *  Armin Diehl <ad@ardiehl.de>
 ****************************************************************************/
/*
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
 *
 * mai 2000 floppy controller (wd1793 + 2 or 8k buffer)
 */
#include <stdio.h>
#include "m68k.h"
#include "fd.h"
#include "sim.h"

#define MYSELF MSG_FD

fd_t fd;

char wd179x_regNames[][25] = {
	"1793_Cmd",
    "1793_TRACK",
    "1793_SECTOR",
    "1793_DATA",
	"1793_Status",
	""};


void decode_controlLatch (char * dst, UINT8 value) {
    sprintf(dst,"SEL:%d %d MOTOR: %d %d DLOCK: %d %d PRECOMP: %d SIDE: %d",
            value & 1,
            ((value & FLPCONT_SEL1) ? 1 : 0),
            ((value & FLPCONT_MOTOR0) ? 1 : 0),
            ((value & FLPCONT_MOTOR1) ? 1 : 0),
            ((value & FLPCONT_DLOCK0) ? 1 : 0),
            ((value & FLPCONT_DLOCK1) ? 1 : 0),
            ((value & FLPCONT_PRECOMP) ? 1 : 0),
            ((value & FLPCONT_SIDE) ? 1 : 0));
}


void decode_optionsLatch (char *dst, UINT8 value) {
    sprintf(dst,"BUFWR: %d CMD+: %d ENBINTR+: %d ENBDRQ+: %d HDSD: %d FM_MFM: %d FRST: %d",
        ((value & FLPOPT_BUFWR) ? 1 : 0),
        ((value & FLPOPT_CMD) ? 1 : 0),
        ((value & FLPOPT_ENBINTR) ? 1 : 0),
        ((value & FLPOPT_ENBDRQ) ? 1 : 0),
        ((value & FLPOPT_HD_SD) ? 1 : 0),
        ((value & FLPOPT_FM_MFM) ? 1 : 0),
        ((value & FLPOPT_FRES) ? 1 : 0));
}


char * wd1793cmds[] = {
	"WD179X_RESTORE",
	"WD179X_SEEK",
	"WD179X_STEP",
	"WD179X_STEP_U",
	"WD179X_STEP_IN",
	"WD179X_STEP_IN_U",
	"WD179X_STEP_OUT",
	"WD179X_STEP_OUT_U",
	"WD179X_READ_REC",
	"WD179X_READ_RECS",
	"WD179X_WRITE_REC",
	"WD179X_WRITE_RECS",
	"WD179X_READ_ADDR",
	"WD179X_FORCE_INTR",
	"WD179X_READ_TRACK",
	"WD179X_WRITE_TRACK"
};


// Status Transfer Control (13L) (read)
void decode_flpstat (char *dst, UINT8 value) {
    sprintf(dst,"BUSY: %d IDX: %d ENINTR: %d ENDRQ: %d INTRA: %d DRQA: %d RST: %d",
        (value & FLPSTAT_BUSY) ? 1 : 0,
        (value & FLPSTAT_IDXA) ? 1 : 0,
        (value & FLPSTAT_ENINTR) ? 1 : 0,
        (value & FLPSTAT_ENDRQ) ? 1 : 0,
        (value & FLPSTAT_INTRA) ? 1 : 0,
        (value & FLPSTAT_DRQA) ? 1 : 0,
        (value & FLPSTAT_FRST) & 1);
}

int fd_getIndexPulse() {
	// only if a drive is selected and motor is on
	UINT8 c;

	int drive = -1;
	c = FLPCONT_SEL0 | FLPCONT_MOTOR0;
	if ((fd.flpcont_13K & c) == c) drive = 0;
	else {
		c = FLPCONT_SEL1 | FLPCONT_MOTOR1;
		if ((fd.flpcont_13K & c) == c) drive = 1;
	}
	if (drive < 0) return 0;
	// get the index pulse from the # instructions counter
	// not accurate but some kind of pulse ;-)
	return ((m68k_instruction_count & 0x07ff0) > 0);
}

/*
   BFSID8079A
   Figure 10-2 Logic Diagram, Central Microprocessor Board (Sheet 47 of 58)
   Bit 0 = Busy
       1 = Index - A
       2 = ENBINTR+
       3 = ENBDRQ+
       4 = INTR+A
       5 = DRQ+A (Data ReQuest)
           DRQ+A is asserted when the floppy disk controller chip is ready to transfer a byte of data to or from the buffer.
       6 = N/C
       7 = PRST-
*/

UINT8 fd_getFlpStat13L() {
	UINT8 status;
	status = fd.flpstat_13L;
	if (fd_getIndexPulse()) {
		status |= FLPSTAT_IDXA;
	} else {
		status &= ~FLPSTAT_IDXA;
    }
	return status;
}


unsigned int fd_read_byte(unsigned int address, int flags) {

	int bufPos;
	int regNum;
    char s[255];
    UINT8 flpstat_13L;

	switch FD_AREA(address) {
		case FD_FLPOPT:	MSG (MSGC_ERR,MYSELF,MSG_WRITEB,"read of write only addr %08x (Floppy option latch)",address);
						return 0xff;
		case FD_STAT:   flpstat_13L = fd_getFlpStat13L();
						// TODO: Busy flag seems to be missing here
						//if (fd.regs[WD1793_R_STAT] & FLG_BUSY) flpstat_13L |= FLG_BUSY; // to pass Test 5
						if (fd.regs[WD1793_R_STAT] & FLG_BUSY) flpstat_13L |= FLPSTAT_BUSY;
						else flpstat_13L &= ~FLPSTAT_BUSY;

						// no fdc or diskette is wite protected
						//if (!fd.regs[WD1793_R_STAT] & FLG_BUSY) flpstat_13L &= 4;
		                decode_flpstat (s,flpstat_13L);
                        MSG (MSGC_INFO,MYSELF,MSG_READB," %08x (Floppy status) %02x %s",address,flpstat_13L,s);
						return flpstat_13L;
		case FD_CONT:	MSG (MSGC_ERR,MYSELF,MSG_WRITEB,"read of write only addr %08x (Floppy control latch)",address);
						return 0xff;
		case FD_WD1793: regNum = (address >> 1) & 0x03;
		                if (regNum == WD1793_STAT) regNum = WD1793_R_STAT;  // 0=stat(r) and cmd(w)
		                MSG (MSGC_DEV,MYSELF,MSG_READB,"%08x (wd1793) regNum %d (%s), returning 0x%02x",address,regNum,wd179x_regNames[regNum],fd.regs[regNum]);
						return fd.regs[regNum];
		case FD_BUFF:											// 7AXXXX Both Floppy buffer READ/WRITE as in the service manual
		case FD_BUFF2:	bufPos = address & FD_BUFFER_MASK;		// but fdfs is using 7Bxxxx ?
						return fd.flpBuf[bufPos];
		default:		MSG (MSGC_ERR,MYSELF,MSG_READB,"%08x",address);
	}
	return 0xff;
}

unsigned int fd_read_word(unsigned int address, int flags) {
	return fd_read_byte(address,flags);
}


void fd_write_cmd (UINT8 value) {
  char params[255];
  int stepping;

  params[0] = 0;   /* type 3 commands never filled this in, uninitialised %s below */
  char cmdName[20];  // AD 23.10.2020, was to small

  fd.regs[WD1793_R_CMD] = value;
  int cmd = value & 0xf0;
  int type = 1;
  if ((cmd & 0xf0) == 0xd0) type = 4;
  else if (cmd >= 0xc0) type = 3;
  else if (cmd >= 0x80) type = 2;

  switch (type) {
	case 1: {
		switch (cmd & 0x03) {
			case 0 : { stepping = 6; break; }
			case 1 : { stepping = 12; break; }
			case 2 : { stepping = 20; break; }
			case 3 : { stepping = 30; break; }
		}
		if (cmd <= 1) sprintf(params,"h(motor on): %d, V: %d, steprate: %d ms\n",
		  (value >> WS1793_CF_MOTOR) & 1,(value >> WS1793_CF_VERIFY) & 1,stepping);
		else
		  sprintf(params,"h(motor on): %d, V: %d, steprate: %d ms update track: %d\n",
		  (value >> WS1793_CF_MOTOR) & 1,(value >> WS1793_CF_VERIFY) & 1,stepping,(value >> WS1793_CF_UPDATE) & 1);
		break;
	}
	case 2: { sprintf(params,"m(muli sector): %d, s(side compare): %d, E:%d, C: %d, P:%d\n",value >> WS1793_CF_MULTSEC & 1,value >> WS1793_CF_SIDECOMP & 1,value >> WS1793_CF_E & 1,value >> WS1793_CF_C & 1,value >> WS1793_CF_P & 1); break; }
	case 3: { sprintf(params,"m(multi sector): %d, s(side compare): %d, E: %d\n",value >> WS1793_CF_MULTSEC & 1,value >> WS1793_CF_SIDECOMP & 1,value >> WS1793_CF_E & 1); break; }
	case 4: { sprintf(params,"Int cond: 0x%02x",value & 0x0f); break; }
  }
  cmdName[0] = '\0';
  switch (cmd) {
      case 0x00 : { strcpy(cmdName,"Restore"); break; }
      case 0x10 : { strcpy(cmdName,"Seek"); break; }
      case 0xC0 : { strcpy(cmdName,"Read Address"); break; }
      case 0xE0 : { strcpy(cmdName,"Read Track"); break; }
      case 0xF0 : { strcpy(cmdName,"Write Track"); break; }
      case 0xD0 : { strcpy(cmdName,"Force Interrupt"); break; }
  }

  if (cmdName[0] == 0) {
	  cmd = value & 0xe0;
	  switch (cmd) {
		  case 0x20 : { strcpy(cmdName,"Step"); break; }
		  case 0x40 : { strcpy(cmdName,"Step-in"); break; }
		  case 0x60 : { strcpy(cmdName,"Step-out"); break; }
		  case 0x80 : { strcpy(cmdName,"Read Sector"); break; }
		  case 0xA0 : { strcpy(cmdName,"Write Sector"); break; }
	  }
  }
  MSG (MSGC_FUNC|MSGC_NOPC,MYSELF,MSG_NONE,"cmd: 0x%02x (%s %s)\n",value,cmdName,params);
}


void fd_exec_command(UINT8 cmd) {
    fd_write_cmd(cmd);
    switch(cmd & 0xf0) {
		case WD179X_RESTORE:
		case WD179X_SEEK:
		case WD179X_STEP:
		case WD179X_STEP_U:
		case WD179X_STEP_IN:
		case WD179X_STEP_IN_U:
		case WD179X_STEP_OUT:
		case WD179X_STEP_OUT_U:
            fd.regs[WD1793_R_STAT] = FLG_BUSY | FLG_HEADLOAD;
            fd_setContinueCounter (FD_SEEK_EXEC_TIME);
            fd.cmdRunning = 1;                       // fd_processContinue will be called from the main loop later
			MSG (MSGC_FUNC,MYSELF,MSG_NONE,"started seek %02x (wd1793)",cmd);
			fd_genInterrupt (WD1793_CMD_START);      // in case this int is enabled
			break;
		case WD179X_FORCE_INTR:                      // aborts current command and resets busy status as well
			fd.intFlags = cmd & 0x0f;
			fd.cmdRunning = 0;                       // abort command
			fd.regs[WD1793_R_STAT] &= ~FLG_BUSY;     // no longer busy
			if (fd.intFlags & WD1793_INT_IMMEDIATE)  // and gen int if requested
				fd_genInterrupt (WD1793_IMMEDIATE);
			break;

		default: MSG (MSGC_NOTIMP|MSGC_NOPC,MYSELF,MSG_NONE,"cmd %02x (wd1793)",cmd);
    }

}




void fd_write_byte(unsigned int address, unsigned int value, int flags) {
	int bufPos;
	int regNum;
    char st[255];

	switch FD_AREA(address) {
		case FD_FLPOPT: decode_optionsLatch (&st[0],value);
						// TODO: reset, fdfs writes 0x00, 0x80
			// AD 23.10.2020: removed break here
                        MSG (MSGC_INFO /*+MSGC_BREAK */,MYSELF,MSG_WRITEB,"%02x (%s) to %08x (Floppy option latch 13J)",value,st,address);
                        fd.flpopt_13J = value;
                        if (!(value & (1 >> FLPOPT_FRES))) { // FRES=0 perform reset, rampos and 13k
						  fd.bufferPos = 0;
						  fd.flpcont_13K = 0xff;
                        }
                        // 13J, we need to transfer to 13L: Bit 7(FRST-) 3(ENDRQ+) and 2(ENINTR+)
                        UINT8 mask = FLPSTAT_FRST | FLPSTAT_ENDRQ | FLPSTAT_ENINTR;
                        fd.flpstat_13L &= ~mask;
                        fd.flpstat_13L |= (value & mask);
						return;
		case FD_STAT:	MSG (MSGC_ERR,MYSELF,MSG_WRITEB,"Write %02x to read only register %08x (Floppy status)",value,address);
						return;
		case FD_CONT:   decode_controlLatch(&st[0],value);
                        MSG (MSGC_INFO,MYSELF,MSG_WRITEB,"%02x (%s) to %08x (Floppy contol latch)",value,st,address);
                        fd.flpcont_13K = value;
						return;
		case FD_WD1793:	regNum = (address >> 1) & 0x03;
		                fd.regs[regNum] = value;
		                if (regNum == WD1793_CMD) {
							fd_exec_command(value);
						} else {
			                MSG (MSGC_DEV,MYSELF,MSG_WRITEB,"%02x to %08x (wd1793) regNum %d (%s)",value,address,regNum,wd179x_regNames[regNum]);
						}
						return;
		case FD_BUFF:
		case FD_BUFF2:	bufPos = address & FD_BUFFER_MASK;
						fd.flpBuf[bufPos] = value;
						return;
		default:		MSG (MSGC_ERR,MYSELF,MSG_WRITEB,"%02x to %08x",value,address);
	}
}

void fd_write_word(unsigned int address, unsigned int value, int flags) {
	fd_write_byte(address,value & 0x00ff,flags);
}


void fd_pulse_reset(void) {
	memset(&fd,0,sizeof(fd));
	fd.flpopt_13J = 0xff;
    fd.flpcont_13K = 0xff;
}


void fd_genInterrupt(int kind) {
    // TODO: check if ints are enabled on CMB - Status Transfer Control (13L)
    // if not, exit here
    //return;
	// is command complete interrupt enabled in the wd1793 ?
	switch (kind) {
        case WD1793_CMD_START:
        	fd.flpstat_13L &= ~FLPSTAT_INTRA;     // no int (from 1793 to 13L, set after command complete)
            if (fd.intFlags & WD1793_INT_NOTREADY) {
                MSG (MSGC_INFO|MSGC_NOPC,MYSELF,MSG_NONE,"generating fd intr READY->NOT READY");
                m68k_pulse_interrupt (FD_INTNO);
            }
            break;
        case WD1793_CMD_COMPLETE:
        	fd.flpstat_13L |= FLPSTAT_INTRA;
            //if (fd.flpopt_13J & FLPOPT_ENBINTR) {
            // the kernel expects interrupts always even when disabled via 13L, fdfs always enables FLPOPT_ENBINTR even when interrupts are disabled
            // so always generate an interrupt on complete
                MSG (MSGC_INFO|MSGC_NOPC,MYSELF,MSG_NONE,"generating fd intr CPMPLETE");
                m68k_pulse_interrupt (FD_INTNO);
            //}
            break;
        case WD1793_INDEX:
            // TODO: needed at all ?
            if (fd.intFlags & WD1793_INT_INDEX) {
                MSG (MSGC_INFO|MSGC_NOPC,MYSELF,MSG_NONE,"generating fd intr INDEX");
                m68k_pulse_interrupt (FD_INTNO);
            }
            break;
        case WD1793_IMMEDIATE:
            if (fd.intFlags & WD1793_INT_IMMEDIATE) {
                MSG (MSGC_INFO|MSGC_NOPC,MYSELF,MSG_NONE,"generating fd intr IMMEDIATE");
                m68k_pulse_interrupt (FD_INTNO);
            }
            break;
	}
}

void fd_processContinue(void) {  /* called after n instructions if a ws1793 command is active */
    int cmd;
	int newTrack;

    if (fd.cmdRunning) {
        fd.cmdRunning = 0;
        cmd = fd.regs[WD1793_R_CMD] & 0xf0;
        switch(cmd) {
            case WD179X_RESTORE:
            case WD179X_SEEK:
            case WD179X_STEP:
            case WD179X_STEP_IN:
            case WD179X_STEP_OUT:
            case WD179X_STEP_U:
            case WD179X_STEP_IN_U:
            case WD179X_STEP_OUT_U:
                fd.regs[WD1793_R_STAT] = FLG_HEADLOAD;
                if (cmd == WD179X_STEP) {  // set to last dir
                  if (fd.lastStepDirection > 0) cmd = WD179X_STEP_IN;
                  else if (fd.lastStepDirection < 0) cmd = WD179X_STEP_OUT;
                } else
                if (cmd == WD179X_STEP_U) {  // set to last dir
                  if (fd.lastStepDirection > 0) cmd = WD179X_STEP_IN_U;
                  else if (fd.lastStepDirection < 0) cmd = WD179X_STEP_OUT_U;
                }
                switch (cmd) {
					case WD179X_SEEK:  // target track in data reg
						newTrack = fd.regs[WD1793_R_DATA];
						printf("seek, new track %d\n",newTrack);
					    if (newTrack > fd.currTrack) fd.lastStepDirection = 1;
					    else if (newTrack < fd.currTrack) fd.lastStepDirection = -1;
					    fd.currTrack = newTrack;
					    fd.regs[WD1793_R_TRACK] = newTrack;
					    break;
                    case WD179X_RESTORE:
                        fd.currTrack = 0;
                        fd.regs[WD1793_R_TRACK] = 0;
                        break;
                    case WD179X_STEP_IN:
                    case WD179X_STEP_IN_U:
                        if (fd.currTrack < 255) fd.currTrack++;
                        if (cmd != WD179X_STEP_IN_U) fd.regs[WD1793_R_TRACK] = fd.currTrack;
                        fd.lastStepDirection = 1;
                        break;
                    case WD179X_STEP_OUT:
                    case WD179X_STEP_OUT_U:
                        if (fd.currTrack > 0) fd.currTrack--;
                        if (cmd != WD179X_STEP_OUT_U) fd.regs[WD1793_R_TRACK] = fd.currTrack;
                        fd.lastStepDirection = -1;
                        break;
                }

                if (fd.currTrack == 0) {
                    fd.regs[WD1793_R_STAT] |= FLG_TRACK0;
                }
                fd.regs[WD1793_R_STAT] &= ~FLG_BUSY;     // no longer busy
                fd_genInterrupt (WD1793_CMD_COMPLETE);
                MSG (MSGC_FUNC|MSGC_NOPC,MYSELF,MSG_NONE,"finished seek, wd status reg 0x%02x",fd.regs[WD1793_R_STAT]);
                break;

        default: MSG (MSGC_NOTIMP,MYSELF,MSG_NONE,"cmd %02x (wd1793)",cmd);
        }
    }
}

int fd_attach_image (int unit, const char * name) {
    long sz;
    fd_unitRegs_t *fdu;

    if ((unit < 0) || (unit >= FD_MAX_DRIVES)) return 0;
    fdu = &fd.units[unit];

    if (fdu->img) { fclose(fdu->img); fdu->img = NULL; fdu->imgBlocks = 0; }
    if (!name) return 1;
    fdu->imgReadonly = 0;
    fdu->img = fopen(name,"r+b");
    if (!fdu->img) {
        fdu->img = fopen(name,"rb");
        if (fdu->img) fdu->imgReadonly = 1;
    }
    if (!fdu->img) {
        printf("fd%d: cannot open '%s'\n",unit,name);
        return 0;
    }
    fseek(fdu->img,0,SEEK_END);
    sz = ftell(fdu->img);
    if (sz <= 0) { printf("fd,%d: '%s' is empty\n",unit,name); fclose(fdu->img); fdu->img=NULL; return 0; }
    fdu->imgBlocks = (UINT32)(sz / FD_SECTOR_SIZE);
    strncpy(fdu->imgName,name,sizeof(fdu->imgName)-1);
    printf("fd,%d: attached '%s', %u blocks (%.1f MB)%s\n",unit,name,fdu->imgBlocks,
            (double)fdu->imgBlocks * FD_SECTOR_SIZE / 1048576.0,
            fdu->imgReadonly ? " read only" : "");
    return 1;
}


/******************************************************************************
 * Commands
 ******************************************************************************/

 void fd_image (int numArgs, struct args_t *args) {
    int unit = 0;

    if (numArgs < 1) {
		for (unit=0; unit < FD_MAX_DRIVES; unit++)
			printf("fd,%d image: %s (%u blocks)\n",unit,
				fd.units[unit].img ? fd.units[unit].imgName : "<none>",fd.units[unit].imgBlocks);
        return;
    }
    if (numArgs > 1) unit = args[1].value;
    if (!fd_attach_image(unit,args[0].txt)) printf("unable to attach image\n");
}


void fd_imageRemove (int numArgs, struct args_t *args) {
    int unit = 0;

    if (numArgs > 0) unit = args[0].value;
    if (!fd_attach_image(unit,NULL)) printf("unable to detach image\n");
}



void fd_showRegs(int numArgs, struct args_t *args) {
	int i;
    char s[255];

	for (i=0;i<FD_MAX_REGISTERS; i++)
		printf("%2d %s: %02x\n",i,wd179x_regNames[i],fd.regs[i]);

    decode_optionsLatch (s,fd.flpopt_13J);
	printf("flpopt_13J @ 0x%08x): 0x0%2x (%s)\n",FD_ADDR_FLPOPT,fd.flpopt_13J,s);
    decode_flpstat (s,fd.flpstat_13L);
	printf("flpstat_13L @ 0x%08x): 0x%02x (%s)\n",FD_ADDR_FLPSTAT,fd.flpstat_13L,s);
    decode_controlLatch(s,fd.flpcont_13K);
	printf("flpcont_13K @ 0x%08x): 0x%02x (%s)\n",FD_ADDR_FLPCONT,fd.flpcont_13K,s);
	printf("lastStepDirection: %d, currTrack: %d\n",fd.lastStepDirection,fd.currTrack);
}




void fd_help (int numArgs, struct args_t *args);

struct cmds_t fdCmds[] =
    {
	{ "image",      fd_image,       0,3,0,"image <file> [unit] - attach a raw disk image"},
    { "detach",     fd_imageRemove, 1,2,0,"detach [unit] - remove an attached disk image"},
    { "registers"  ,fd_showRegs,    0,0,0,"show fd registers"},
    { "?"          ,fd_help,        0,0,0,""},
    { "help"       ,fd_help,        0,0,0,"show this help"},
    { ""           ,  NULL,         0,0,0,""}
};

void fd_help (int numArgs, struct args_t *args) {
	showHelp ("fd help commands",fdCmds,0);
}


int fd_dbgCmd(int numArgs, struct args_t * args) {
	return findAndExecCommand (args[0].txt,fdCmds,numArgs-1,&args[1]);
}


int fd_save_state(FILE * f) {
    STATEWRITEVARS("fd_");

    STATEWRITE(id,f);
    STATEWRITELEN(fd,f);
    return 1;
}

int fd_load_state(FILE * f) {
    STATEREADVARS("fd_");

    STATEREADID(f);
    STATEREADLEN(fd,f);
    return 1;
}
