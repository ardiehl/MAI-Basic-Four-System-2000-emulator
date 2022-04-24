/***************************************************************************
 *  wd.c
 *
 *  Created: Nov, 22 2011
 *  Changed: Dec, 21 2011
 *  Armin Diehl <ad@ardiehl.de>
 ****************************************************************************/
/*
 * mai 2000/3000 winchester disk (wd) controller definitions
 * see M8158A '5 1/4" Winchester Disk Drive Controller Service Manual'
 */

#include <stdio.h>
#include "m68k.h"
#include "wd.h"
#include "sim.h"
#include "memory.h"

int installedWds[WD_MAX] = {WD0_INSTALLED,WD1_INSTALLED};


wd_regs_t wdr[WD_MAX];



#define MYSELF MSG_WD

/* stat after select                         c2: 1100 0010
 *                                           cb: 1100 1011
 * stat after command(expected??):           cc: 1100 1100
 * expected after all command bytes received e8: 1110 1000
 *                                           48: 0100 1000
 *                                               |||| ||||- 0: Bus error
 *                                               |||| |||-- 1: Output data register empty
 *                                               |||| ||--- 2: Operation complete
 *                                               |||| |---- 3: Input data register full
 *                                               ||||------ 4: scsi bus in reset state
 *                                               |||------- 5: scsi bus in message phase
 *                                               ||-------- 6: scsi bus busy
 *                                               |--------- 7: scsi bus in command, status or message phase
 *  and fb = 1111 1011, then expected e8
 */


/* return 1 if command transfers data to host */
int cmdTransferToHost (int cmd) {
    switch (cmd) {
        //case SCSI_TESTREADY     : return 1;
        //case SCSI_REZEROUNIT    : return 1;
        case SCSI_REQUESTSENSE  : return 1;
        //case SCSI_FORMAT        : return 1;
        case SCSI_READ          : return 1;
        //case SCSI_WRITE         : return 1;
        //case SCSI_SEEK          : return 1;
        //case SCSI_TRANSLATE     : return 1;
        //case SCSI_WRITEBUF      : return 1;
        case SCSI_READBUFRAM    : return 1;
        //case SCSI_MODESELECT    : return 1;
        //case SCSI_MODESENSE     : return 1;
        //case SCSI_STARTSTOP     : return 1;
        case SCSI_RECDIAG       : return 1;
        //case SCSI_SENDDIAG      : return 1;
        case SCSI_READCAPACITY  : return 1;
        case SCSI_READ2         : return 1;
        case SCSI_WRITE2        : return 1;
        //case SCSI_WRITEVERIFY   : return 1;
        //case SCSI_VERIFY        : return 1;
        case SCSI_SEARCH        : return 1;
    }
    return 0;
}

int cmdTransferToController (int cmd) {
    switch (cmd) {
        //case SCSI_TESTREADY     : return 1;
        //case SCSI_REZEROUNIT    : return 1;
        //case SCSI_REQUESTSENSE  : return 1;
        case SCSI_FORMAT        : return 1;
        //case SCSI_READ          : return 1;
        case SCSI_WRITE         : return 1;
        //case SCSI_SEEK          : return 1;
        //case SCSI_TRANSLATE     : return 1;
        case SCSI_WRITEBUF      : return 1;
        //case SCSI_READBUFRAM    : return 1;
        //case SCSI_MODESELECT    : return 1;
        //case SCSI_MODESENSE     : return 1;
        //case SCSI_STARTSTOP     : return 1;
        //case SCSI_RECDIAG       : return 1;
        case SCSI_SENDDIAG      : return 1;
        //case SCSI_READCAPACITY  : return 1;
        //case SCSI_READ2         : return 1;
        case SCSI_WRITE2        : return 1;
        case SCSI_WRITEVERIFY   : return 1;
        //case SCSI_VERIFY        : return 1;
        //case SCSI_SEARCH        : return 1;
    }
    return 0;
}


int cmdNoDataTransfer (int cmd) {
    if ( (cmdTransferToController(cmd) == 0) && (cmdTransferToHost(cmd)==0) ) return 1;
    return 0;
}


void processScsiNextPhase (wd_regs_t * wd) {
    int unit = (wd->scsiBuf[1] >> 5) & 0x07;
    int blockNo;
    int class;
    int numBlocks;

    if (wd->state == SCSI_S_IDLE) return;

    if (wd->state == SCSI_S_COMPLETE) {
        msgout (MSGC_WARN,MYSELF,MSG_NONE,"end of scsi command %02x, sense byte %02x",wd->scsiBuf[0] & 0x1f,wd->replyBuffer[0]);
        wd->statusReg = 0x48;
        wd->state = SCSI_S_READRESULTS;
        return;
    } else
    if (wd->state == SCSI_S_READRESULTS) {
        msgout (MSGC_WARN,MYSELF,MSG_NONE,"end of read results phase");
        wd->statusReg = 0xcc;
        wd->state = SCSI_S_MESSAGEBYTE;
        return;
    } else
    if (wd->state == SCSI_S_MESSAGEBYTE) {
        msgout (MSGC_WARN,MYSELF,MSG_NONE,"message byte (00) was read, set status E8");
        wd->statusReg = 0xe8;
        wd->state = SCSI_S_ENDOFCOMMAND;
        wd->stateCounter = 2;
        return;
    } else
    if (wd->state == SCSI_S_ENDOFCOMMAND) {
        msgout (MSGC_WARN,MYSELF,MSG_NONE,"end of transaction, deselect");
        wd->statusReg = 0;
        wd->state = SCSI_S_IDLE;
        wd->selected = 0;
        wd->hostWriteReg = 0; /* this is returned as status if not selected */
        return;
    }

    wd->scsiCheck=0;
    wd->readInpReg = 0x00;  /* message byte, must be zero */
    wd->replyBytePos=0;
    wd->replyBytesLeft = 1;
    memset(wd->replyBuffer,0,sizeof(wd->replyBuffer));
    class = (wd->scsiBuf[0] >> 5) & 0x07;
    if (class == 0) {
        numBlocks = wd->scsiBuf[4];
    } else
    if (class == 1) {
        numBlocks = ((wd->scsiBuf[7] << 8) & 0xff00) | wd->scsiBuf[8];
    } else {
        msgout (MSGC_ERR|MSGC_BREAK,MYSELF,MSG_NONE,"Invalid class %02x in scsi command, command byte: %02x",class,wd->scsiBuf[0]);
        wd->replyBytesLeft = 1; wd->replyBuffer[0] = 0x02;
        wd->state = SCSI_S_READRESULTS;
        processScsiNextPhase (wd);
        return;
    }

    msgout (MSGC_FUNC,MYSELF,MSG_NONE,"Executing scsi command %02x unit %d blocks %d (%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x)",wd->scsiBuf[0] & 0x1f,unit,numBlocks,
                wd->scsiBuf[0],wd->scsiBuf[1],wd->scsiBuf[2],wd->scsiBuf[3],wd->scsiBuf[4],wd->scsiBuf[5],wd->scsiBuf[6],wd->scsiBuf[7],wd->scsiBuf[8],wd->scsiBuf[9]);
    if (wd->ctlReg2 & WD_CTL_SEQEN)
        msgout (MSGC_ERR|MSGC_BREAK,MYSELF,MSG_NONE,"DMA not yet implemented");
    switch (wd->scsiBuf[0] & 0x1f) {
        case SCSI_TESTREADY:        wd->replyBytesLeft = 1;
									wd->replyBuffer[0]=0;
                                    if (unit > 0) wd->replyBuffer[0]=0x02;  // check=error

                                    wd->statusReg = 0xcc;  // diskfs expects cc     0xe8; //0xcc;
                                    //wd->state = SCSI_S_IDLE;
                                    //return;
                                    break;
		case SCSI_REZEROUNIT:
        case SCSI_REQUESTSENSE:     wd->replyBytesLeft = 4;
                                    //wd->statusReg = WD_STAT_SBUSY | WD_STAT_INPFULL;  /* data available */
                                    //wd->statusReg = 0xcc;  //Test 12:0x48
                                    wd->statusReg = 0x48;
                                    if (unit > 0) wd->replyBuffer[0] = 0x02;
                                    if (wd->replyBuffer[0] != 0) {
                                        wd->state = SCSI_S_READRESULTS;
                                        wd->replyBytesLeft = 1;
                                        return;
                                    }
                                    break;
        case SCSI_READ:             blockNo = ((wd->replyBuffer[1] & 0x1f) << 16) |
                                              ((wd->replyBuffer[2]) << 8) |
                                                wd->replyBuffer[3];
                                    msgout (MSGC_ERR,MYSELF,MSG_NONE,"Read block %6x not yet implemented",blockNo);
                                    wd->replyBytesLeft = 4;
                                    //wd->statusReg = WD_STAT_SBUSY | WD_STAT_INPFULL;  /* data available */
                                    wd->statusReg = 0xcc;
                                    break;
        default:                    msgout (MSGC_WARN,MYSELF,MSG_NONE,"scsi command %02x not implemented",wd->scsiBuf[0] & 0x1f);
                                    wd->replyBytesLeft = 1;
                                    wd->replyBuffer[0] = 0x02;
                                    wd->statusReg = 0xcc;
    }
    wd->state = SCSI_S_COMPLETE;
    //wd->stateCounter = 2;

}

int processScsiCommand(wd_regs_t * wd);

void processCommand(wd_regs_t * wd) {
    msgout (MSGC_FUNC,MYSELF,MSG_NONE,"end of command %02x, replyBytesLeft:%d",wd->currCommand,wd->replyBytesLeft);
    switch (wd->currCommand) {
        case CMD_RESET     :        //wd->statusReg &= ~(WD_STAT_BUSY);
                                    wd->statusReg = 0; /* expected by test 5 */
                                    break;
        case CMD_SCSIRESET :        wd->statusReg &= ~(WD_STAT_SRESET);
                                    break;
        case CMD_RESET_OUTREGFULL : wd->statusReg |= WD_STAT_OUTEMPTY;
                                    wd->currCommand = CMD_PROCESS_SCSICMD;
                                    wd->cmdCounter = 2;
                                    break;
        case CMD_PROCESS_SCSICMD:   processScsiCommand(wd);
                                    break;
        case CMD_SET_INPFULL:       if (wd->replyBytesLeft) wd->statusReg |= WD_STAT_INPFULL;
                                    else {
                                        if(wd->state == SCSI_S_READRESULTS) processScsiNextPhase(wd);
                                        else
                                            msgout (MSGC_ERR|MSGC_BREAK,MYSELF,MSG_NONE,"Invalid state %d",wd->state);
                                    }
#if 0
                                    //else wd->statusReg = 0xcc; // test12:0xcc(0x03 sense), test14:0xe8(0x00 testready)
                                    else if (cmdTransferToHost(wd->scsiBuf[0] & 0x1f)) wd->statusReg = 0xCC; // test12: 0x03,sense
                                    else
                                    if (cmdNoDataTransfer(wd->scsiBuf[0] & 0x1f)) wd->statusReg = 0xE8; // test14: 0x00,testready
                                    else msgout (MSGC_ERR|MSGC_BREAK,MYSELF,MSG_NONE,"dont know required status");
#endif
                                    break;

    }
}


void wd_processContinue(void) {  /* called each n instructions */
  if (wdr[0].stateCounter) { wdr[0].stateCounter--; if (wdr[0].stateCounter==0) processScsiNextPhase(&wdr[0]); }
  if (wdr[1].stateCounter) { wdr[1].stateCounter--; if (wdr[1].stateCounter==0) processScsiNextPhase(&wdr[1]); }
  if (wdr[0].cmdCounter) { wdr[0].cmdCounter--; if (wdr[0].cmdCounter==0) processCommand(&wdr[0]); }
  if (wdr[1].cmdCounter) { wdr[1].cmdCounter--; if (wdr[1].cmdCounter==0) processCommand(&wdr[1]); }
}



int processScsiCommand(wd_regs_t * wd) {
    int class,len;

    if (wd->scsiIdx < 6) return 0;
    class = (wd->scsiBuf[0] >> 5) & 0x07;
    switch (class) {
        case 0: len = 6; break;
        case 1: len = 10; break;
        default: msgout (MSGC_ERR,MYSELF,MSG_NONE,"invalid scsi command class %d (cmd=%02x)",class,wd->scsiBuf[0]);
                 return 0;
    }
    if (wd->scsiIdx == len) {
        //printf("--------- got scsi command\n");
        wd->state = SCSI_S_MESSAGE;  /* transfering message to scsi drive */
        wd->statusReg |= 0x20;       /* bus in message phase */
        wd->stateCounter = WD_PHASE_COUNT;
    }
        //wd->statusReg = WD_STAT_SBUSY | WD_STAT_INPFULL;   /* data available */

    return 0;
}

void decodeCtlReg (UINT8 ctlReg, char * tx) {
    char txt[255];
    txt[0]=0; tx[0]=0;
    if (ctlReg & WD_CTL_SRST)    strcat(txt,"SSRST+ ");
    if (ctlReg & WD_CTL_LED)     strcat(txt,"LED+ ");
    if (ctlReg & WD_CTL_INTEN)   strcat(txt,"INTEN+ ");
    if (ctlReg & WD_CTL_SEQEN)	 strcat(txt,"DMA+ ");
    if (ctlReg & WD_CTL_INTEND0) strcat(txt,"INTEND0+ ");
    if (ctlReg & WD_CTL_INTEND1) strcat(txt,"INTEND1+ ");
    if (ctlReg & WD_CTL_INTD0)	 strcat(txt,"INTD0+ ");
    if (ctlReg & WD_CTL_INTD1)   strcat(txt,"INTD1+ ");
    if(txt[0] != 0) {
        txt[strlen(txt)-1] = 0;
        sprintf(tx,"[%s]",txt);
    }
}

void decodeStatusReg(UINT8 statusReg, char * tx) {
    char txt[255];
    txt[0]=0; tx[0]=0;

    if (statusReg & WD_STAT_SCMD)       strcat(txt,"SCMD+ ");
    if (statusReg & WD_STAT_SBUSY)      strcat(txt,"SBUSY+ ");
    if (statusReg & WD_STAT_SMSG)       strcat(txt,"SMSG+ ");
    if (statusReg & WD_STAT_SRESET)     strcat(txt,"SRES+ ");
    if (statusReg & WD_STAT_INPFULL)    strcat(txt,"INPFULL+ ");
    if (statusReg & WD_STAT_OPCOMP)     strcat(txt,"COMPLETE+ ");
    if (statusReg & WD_STAT_OUTEMPTY)   strcat(txt,"OEMPTY+ ");
    if (statusReg & WD_STAT_BUSERR)     strcat(txt,"BUSERR+ ");
    if (statusReg & WD_CTL_SRST)        strcat(txt,"SRST+ ");
    if(txt[0] != 0) {
        txt[strlen(txt)-1] = 0;
        sprintf(tx,"[%s]",txt);
    }
}


unsigned int wd_read_byte(unsigned int address, int flags) {
#ifdef WD_DISABLE
    BUSERROR(flags,address,MSG_READB);
#else
    int regNo;
    wd_regs_t * wd;
    UINT8 value;
    char tx[255];

    wd = NULL;
    if (ADDR_IS_WD0(address)) wd=&wdr[0]; else
    if (ADDR_IS_WD1(address)) wd=&wdr[1];
    regNo = WD_ADDR_TO_REG(address);

    if (wd) {
        if (!(wd->installed)) { BUSERROR(flags,address,MSG_READB); }
        else
        switch (regNo) {             /* not sure if DMA and INTVEC regs are readable -> 01/2021: tested on real hw, they are not, always FF */
            case WD_REG_DMA_HI		: //value = ((wd->dmaAddress >> 16) & 0xff);
                                      //msgout (MSGC_INFO,MYSELF,MSG_READB,"%08x returning DMA H %02x",address,value);
                                      //return value;
                                      msgout (MSGC_ERR,MYSELF,MSG_READB,"%08x attempt ro read write only reg DMA HI",address);
                                      break;
            case WD_REG_DMA_MID		: //value = ((wd->dmaAddress >> 8) & 0xff);
                                      //msgout (MSGC_INFO,MYSELF,MSG_READB,"%08x returning DMA M %02x",address,value);
                                      //return value;
                                      msgout (MSGC_ERR,MYSELF,MSG_READB,"%08x attempt ro read write only reg DMA MID",address);
                                      break;
            case WD_REG_DMA_LOW		: //value = (wd->dmaAddress & 0xff);
                                      //msgout (MSGC_INFO,MYSELF,MSG_READB,"%08x returning DMA L %02x",address,value);
                                      //return value;
                                      msgout (MSGC_ERR,MYSELF,MSG_READB,"%08x attempt ro read write only reg DMA LO",address);
                                      break;
            case WD_REG_INTVEC		: value = wd->intVector;
                                      msgout (MSGC_INFO,MYSELF,MSG_READB,"%08x returning int vector %02x",address,value);
                                      return value;
            case WD_REG_INTVEC2     : /*value = wd->intVectorError;*/
                                      value = wd->intVector;
                                      msgout (MSGC_INFO,MYSELF,MSG_READB,"%08x returning error vector %02x",address,value);
                                      return value;
            case WD_REG_CTL2        : value = wd->ctlReg2;
                                      decodeCtlReg(value,tx);
                                      msgout (MSGC_WARN,MYSELF,MSG_READB,"%08x returning control reg2 %02x %s",address,value,tx);
                                      return value;
            case WD_HOST_WRITE		: msgout (MSGC_ERR,MYSELF,MSG_READB,"%08x attempt ro read write only reg HOST WRITE",address);
                                      break;
            case WD_REG_STAT		: //if (wd->selected) value = wd->statusReg; else value = 0;
									  value = wd->statusReg;
                                      if (wd->ctlReg2 & WD_CTL_SRST) {
                                        //printf("WD_CTL_SRST active, returning 0x10 %02x %02x\n",wd->ctlReg2,wd->ctlReg2 & WD_CTL_SRST);
										value = 0x10 | (wd->statusReg & WD_STAT_BUSERR); /* 0x10 during reset line active, bus err flag expected by test 7 */
									  }


                                      /*if ((wd->scsiCheck) && (value == 0x01)) {
                                          wd->scsiCheck=0;
                                          wd->statusReg=0;
                                      }*/

                                      decodeStatusReg(value,tx);
                                      msgout (MSGC_INFO,MYSELF,MSG_READB,"%08x returning statusReg %02x %s (ctlReg2: %02x, selected:%d)",address,value,tx,wd->ctlReg2,wd->selected);
                                      if(wd->state == SCSI_S_MESSAGEBYTE) processScsiNextPhase(wd);

#if 0
                                      if (complete) {
                                          complete = 0;
                                          wd->statusReg = WD_STAT_SBUSY | WD_STAT_INPFULL; // = 0x48;
                                      }

                                      if (value & WD_STAT_OPCOMP) {         /* after complete */
                                        wd->statusReg &= ~WD_STAT_OPCOMP;   /* clear complete flag */
                                        wd->statusReg &= ~WD_STAT_INPFULL;  /* and input register full */
                                        wd->statusReg |= WD_STAT_OUTEMPTY;
                                        complete = 1;
                                      }
#endif
                                      return value;
            case WD_REG_SELECT      : msgout (MSGC_ERR,MYSELF,MSG_READB,"%08x attempt ro read write only reg SELECT",address);
                                      break;
            case WD_REG_READINP		: if (!(wd->selected)) {
                                            // 0002 moved DMA test read to not selected only
                                            //wd->statusReg &= ~WD_STAT_INPFULL;   /* test 12 */
                                            if (wd->dmaTestCount == 2) {
                                                value = (wd->dmaTestWordRead >> 8) & 0xff;
                                                wd->dmaTestCount--;
                                                msgout (MSGC_INFO,MYSELF,MSG_READB,"%08x returning dmaTest (H) %02x",address,value);
                                            } else
                                            if (wd->dmaTestCount == 1) {
                                                value = wd->dmaTestWordRead & 0xff;
                                                wd->dmaTestCount--;
                                                msgout (MSGC_INFO,MYSELF,MSG_READB,"%08x returning dmaTest (L) %02x",address,value);
                                            } else
                                                value = wd->hostWriteReg; /* loopback */

                                      } else {
                                        wd->scsiIdx=0; /* for test 6 */
                                        if (wd->replyBytesLeft) {
                                           value = wd->replyBuffer[wd->replyBytePos];
                                           wd->replyBytesLeft--;
                                           wd->replyBytePos++;
                                           wd->statusReg &= ~WD_STAT_INPFULL;
                                           wd->cmdCounter = 2;
                                           wd->stateCounter = 0;
                                           wd->currCommand=CMD_SET_INPFULL;
                                           if (wd->replyBytesLeft == 0)
                                                msgout (MSGC_INFO,MYSELF,MSG_NONE,"all reply bytes read");
                                        } else {
                                            value = wd->readInpReg;
                                              //if (wd->statusReg == 0xcc) wd->statusReg = 0xe8;      /* test12 message phase e8 after host has read message byte */
                                              //else if (wd->statusReg == 0xe8) wd->statusReg = 0;
                                        }
                                        msgout (MSGC_INFO,MYSELF,MSG_READB,"%08x returning readInpReg %02x",address,value);
                                      }

                                      return value;
            case WD_REG_CLRBUSERR	: msgout (MSGC_ERR,MYSELF,MSG_READB,"%08x attempt ro read write only reg clrBusErr",address);
                                      break;
            default                 : msgout (MSGC_ERR,MYSELF,MSG_READB,"%08x unknown wd register",address);

        }
    } else BUSERROR(flags,address,MSG_READB);
#endif
	return 0xff;
}

unsigned int wd_read_word(unsigned int address, int flags) {
	return wd_read_byte(address,flags) + ((wd_read_byte(address+1,flags) >> 8) & 0x0ff);
}


void wd_write_byte(unsigned int address, unsigned int value, int flags) {
#ifdef WD_DISABLE
    BUSERROR(flags,address,MSG_WRITEB);
#else
    int regNo;
    wd_regs_t * wd;
    char tx[255];
    UINT32 tmp;
    unsigned int addr;

    wd = NULL;
    if (ADDR_IS_WD0(address)) wd=&wdr[0]; else
    if (ADDR_IS_WD1(address)) wd=&wdr[1];
    regNo = WD_ADDR_TO_REG(address);

    if (wd) {
        if (!(wd->installed)) { BUSERROR(flags,address,MSG_READB); }
        else
        switch (regNo) {
            case WD_REG_DMA_HI		: tmp = value << 16;
                                      wd->dmaAddress &= 0x0ffff;
                                      wd->dmaAddress |= tmp;
                                      msgout (MSGC_INFO,MYSELF,MSG_WRITEB,"%02x to %08x (DMA H), dmaReg:%06x (%06x)",value,address,~wd->dmaAddress,((~wd->dmaAddress) & 0xffffff)<<1);
                                      wd->dmaTestCount = 0;
                                      return;
            case WD_REG_DMA_MID		: tmp = value << 8;
                                      wd->dmaAddress &= 0x0ff00ff;
                                      wd->dmaAddress |= tmp;
                                      msgout (MSGC_INFO,MYSELF,MSG_WRITEB,"%02x to %08x (DMA M), dmaReg:%06x (%06x)",value,address,~wd->dmaAddress,((~wd->dmaAddress) & 0xffffff)<<1);
                                      wd->dmaTestCount = 0;
                                      return;
            case WD_REG_DMA_LOW		: wd->dmaAddress &= 0x0ffff00;
                                      wd->dmaAddress |= value;
                                      msgout (MSGC_INFO,MYSELF,MSG_WRITEB,"%02x to %08x (DMA L), dmaReg:%06x (%06x)",value,address,~wd->dmaAddress,((~wd->dmaAddress) & 0xffffff)<<1);
                                      wd->dmaTestCount = 0;
                                      return;
            case WD_REG_INTVEC      : wd->intVector = value;
                                      msgout (MSGC_INFO,MYSELF,MSG_WRITEB,"%02x to %08x (int vector)",value,address);
#if 0
                                      if (!(wd->selected))
                                        if (!(wd->ctlReg2 & WD_CTL_SEQEN))
                    /* loop-test */       wd->intVectorError = value;  // loopback test, TODO: only in bus free phase and dma disabled
#endif
                                      return;
            case WD_REG_INTVEC2     : /*wd->intVectorError = value;*/
                                      /* can this one be written and what it is used for ? */
                                      msgout (MSGC_INFO|MSGC_BREAK,MYSELF,MSG_WRITEB,"%02x to %08x (int vector error)",value,address);
                                      return;
            /* seems to be that this is the register described under WD_REG_CTL cc0007 */
            case WD_REG_CTL2        : decodeCtlReg(value,tx);
                                      msgout (MSGC_WARN,MYSELF,MSG_WRITEB,"%02x %s to %08x ((R/W Ctrl Reg)",value,tx,address);
                                      if (value & WD_CTL_SRST) {
                                        //wd->statusReg |= WD_STAT_SRESET;  /* (0x10 on real machine) scsi bus reset in progress */
                                        //wd->statusReg = 0x10;     // 0001
                                        wd->cmdCounter = 2;       // 0001
                                        wd->state = SCSI_S_IDLE;  // 0001
                                        wd->replyBytesLeft = 0;   // 0001
                                        wd->currCommand=CMD_RESET;
                                        wd->scsiIdx = 0;
                                        wd->selected = 0;           // 0001 (0001 fails test 6)
                                        //msgout (MSGC_INFO,MYSELF,MSG_WRITEB,"%02x to %08x (R/W Ctrl Reg) statusReg: %02x",value,address,wd->statusReg);
                                      } //else
                                      //if (wd->ctlReg2 & WD_CTL_SRST)  // reset no longer active
                                      //      wd->statusReg = 0xc2;  // will fail test 5
                                      #if 0
                                      if (value == 0)  {
                                          //wd->scsiIdx=0; /* for test 6 */
                                          wd->statusReg &= ~WD_STAT_SBUSY; /* for test 13 */
                                      }
                                      #endif // 0
                                      /* if ints are enabled and the bus error flag is set, generate an interrupt (test 9) */
                                      if ( (value & WD_CTL_INTEN) && (!(wd->ctlReg2 & WD_CTL_INTEN)) && (wd->statusReg & WD_STAT_BUSERR) ) {
                                          msgout (MSGC_INFO,MYSELF,MSG_NONE,"Ints enabled while buserr active, raising interrupt");
                                          //wd->intErrCount++;
                                          wd->intCount++;
                                          m68k_pulse_interrupt (WD_INTNO);
                                      }
                                      wd->ctlReg2 = value & 0x3f;  // bit 6+7 are readonly
                                      return;
            case WD_HOST_WRITE		: wd->hostWriteReg = value;
                                      msgout (MSGC_INFO,MYSELF,MSG_WRITEB,"%02x to %08x (host write reg) idx:%d, selected:%d, ctlReg2:%02x",value,address,wd->scsiIdx,wd->selected,wd->ctlReg2);
                                      if (wd->selected) {
                                          if (wd->scsiIdx < WD_SCSICMD_MAX) {
                                              wd->scsiBuf[wd->scsiIdx] = value;
                                              wd->scsiIdx++;
                                              /*processScsiCommand(wd); do this after OUTREGFULL is cleared */
                                              /* Test 12 expects WD_STAT_OUTEMPTY to go to 0 */
                                              wd->cmdCounter = 2;
                                              wd->currCommand=CMD_RESET_OUTREGFULL;
                                              wd->statusReg &= ~(WD_STAT_OUTEMPTY);
                                          } else
                                            msgout (MSGC_ERR,MYSELF,MSG_WRITEB,"scsi command buffer overflow");
                                      } else {      /* 0002 assumption: reset deselects controller */
                                          /* DMA-Test: "dummy" IO write (when DMA is enabled and adapter is in Bus-Free-Phase)
                                             reads from memory to CC000B (WD_REG_READINP) */
                                          /* Assume "Bus-Free-Phase" means adapter not selected */
                                        if (wd->ctlReg2 & WD_CTL_SEQEN) { /* only when DMA is on */
                                            addr = ((~wd->dmaAddress) << 1) & 0x0ffffff;
                                            wd->dmaTestWordRead = sys_read_word(addr,1);
                                            wd->dmaTestCount = 2;
                                            wd->dmaAddress--;   /* is inverted word address */
                                            if (mem_isValidForRead (addr)) {
                                                msgout (MSGC_INFO,MYSELF,MSG_WRITEB,"DMA-Test, read %04x from %08x",wd->dmaTestWordRead,addr);
                                            } else {
                                                msgout (MSGC_INFO,MYSELF,MSG_WRITEB,"BusError during DMA-Test, read %04x from %08x",wd->dmaTestWordRead,addr);
                                                wd->statusReg |= WD_STAT_BUSERR;
                                            }
                                         }
                                      }
                                      return;
            case WD_REG_STAT		: msgout (MSGC_ERR,MYSELF,MSG_WRITEB,"attempt to write %02x to %08x (statusReg)",value,address);
                                      break;
            case WD_REG_SELECT      : wd->selected = value;
                                      wd->scsiIdx=0;
                                      wd->statusReg = 0xc2;
                                      //wd->ctlReg2 |= WD_CTL_SRST; /* reset=inactive */
                                      wd->cmdCounter = 0;
                                      msgout (MSGC_INFO,MYSELF,MSG_WRITEB,"%02x to %08x (selected reg) ctlReg2:%02x",value,address,wd->ctlReg2);
                                      return;
            case WD_REG_READINP		: msgout (MSGC_ERR,MYSELF,MSG_WRITEB,"%02x to read only %08x (ReadInp) ignored",value,address);
                                      break;
            case WD_REG_CLRBUSERR	: msgout (MSGC_INFO,MYSELF,MSG_WRITEB,"%02x to %08x (clrBusErr)",value,address);
                                      /* ?? should we generate an interrups ?? */
                                      wd->statusReg &= ~WD_STAT_BUSERR;
                                      break;
            default                 : msgout (MSGC_ERR,MYSELF,MSG_WRITEB,"%02x to %08x (unknown wd register)",value,address);

        }
    } else BUSERROR(flags,address,MSG_READB);
#endif
}

void wd_write_word(unsigned int address, unsigned int value, int flags) {
    wd_write_byte(address,value & 0xff,flags);
}


void wd_pulse_reset(void) {
    int i;

    memset(&wdr,0,sizeof(wdr));
    for (i=0;i<WD_MAX;i++) {
        wdr[i].statusReg = 0xC2; /*(1 << WD_STAT_OUTEMPTY) | (1 << WD_STAT_OPCOMP);*/
    }
    if (WD0_INSTALLED) wdr[0].installed = 1;
    if (WD1_INSTALLED) wdr[1].installed = 1;
}


int  wd_irq_ack(int level) {
    if (wdr[0].installed) {
        if ((wdr[0].intCount) && (level == WD_INTNO)) {
            wdr[0].intCount--;
            return wdr[0].intVector;
        }
    }
    if (wdr[1].installed) {
        if ((wdr[1].intCount) && (level == WD_INTNO)) {
            wdr[1].intCount--;
            return wdr[1].intVector;
        }
    }
    return M68K_INT_ACK_SPURIOUS;
}


/******************************************************************************
 * Commands
 ******************************************************************************/

void wd_showRegs (int numArgs, struct args_t *args) {
    int i;
    char tx[255];
    char tx2[255];

    for (i=0; i<WD_MAX;i++) {
        if (wdr[i].installed) {
            decodeStatusReg(wdr[i].statusReg,tx);
            decodeCtlReg(wdr[i].ctlReg2,tx2);
            printf("wd%d    status: %02x %s\n" \
               "       vector: %02x  DMA-address: %06x = %08x\n" \
               "     selected: %02x  ReadInpReg: %02x    HostWrite: %02x\n" \
               " stateCounter: %02d       state: %02x\n" \
               "      ctlReg2: %02x %s\n",
                i,wdr[i].statusReg,tx,
                wdr[i].intVector,wdr[i].dmaAddress,(~wdr[i].dmaAddress) & 0xffffff,
                wdr[i].selected,wdr[i].readInpReg,wdr[i].hostWriteReg,
                wdr[i].stateCounter,wdr[i].state,
                wdr[i].ctlReg2,tx2);
        }
    }
}

void wd_help (int numArgs, struct args_t *args);

struct cmds_t wdCmds[] =
{
	{ "registers",	wd_showRegs,    0,0,0,"show wd registers"},
	{ "?",			wd_help,        0,0,0,"show this help"},
	{ "help",		wd_help,        0,0,0,"show this help"},
    { "",  NULL, 0,0,0,""}
};

void wd_help (int numArgs, struct args_t *args) {
	showHelp ("wd help commands",wdCmds,0);
}


int wd_dbgCmd(int numArgs, struct args_t * args) {
	return findAndExecCommand (args[0].txt,wdCmds,numArgs-1,&args[1]);
}


int wd_save_state(FILE * f) {
    STATEWRITEVARS("wd_");

    STATEWRITE(id,f);
    printf("wd_save_state not yet implemented\n");
    return 1;
}

int wd_load_state(FILE * f) {
    STATEREADVARS("wd_");

    STATEREADID(f);
    printf("wd_load_state not yet implemented\n");
    return 1;
}
