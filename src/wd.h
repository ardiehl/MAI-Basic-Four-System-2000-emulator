/***************************************************************************
 *   wd.h
 *
 *  Created: Nov, 22 2011

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
 * mai 2000/3000 winchester disk (wd) controller definitions
 * see M8158A '5 1/4" Winchester Disk Drive Controller Service Manual'
 */

#ifndef WD_H
#define WD_H

#include "sim.h"

//define WD_DISABLE

#define WD0_ADDR		0xCC0000
#define WD1_ADDR		0xCD0000
#define WD_ADDR_MASK	0xFFFF0000


#define WD0_INSTALLED   1
#define WD1_INSTALLED   0
#define WD_MAX          2
// this is the size of the buffer ram of the wd/acb-4000
#define WD_DATABUF_LEN_MAX 1024
// as all incoming data is in scsiBuf as well we need this instead of 255
#define WD_SCSICMD_MAX  WD_DATABUF_LEN_MAX + 20
#define WD_SECTOR_SIZE  512
// max units (drives) per wd
#define WD_MAX_UNITS    2

#define ADDR_IS_WD0(ADDR) ((ADDR & WD_ADDR_MASK) == WD0_ADDR)
#define ADDR_IS_WD1(ADDR) ((ADDR & WD_ADDR_MASK) == WD1_ADDR)

// the loader from the install tape accesses the controller e.g. via cc8007
// tested on 2000, only the lower 4 bits will be decoded ccfff7 is equal to cc0007
#define WD_ADDR_TO_REG(A)	(A & 0x0f)
#define WD_PHASE_COUNT      5
#define WD_CMD_COUNT        2

/* dont know if it is 2 or 4 (both are vectored) */
#define WD_INTNO            2


/* These registers are loaded one byte at a time, with the 1's complement
   of the system address, right shifted once */
#define WD_REG_DMA_HI		0x01
#define WD_REG_DMA_MID		0x02
#define WD_REG_DMA_LOW		0x03

/* interrupt vector register */
#define WD_REG_INTVEC		0x04

/* int vector register ?? Test reads from this port and requires value written to 0x04 ?? */
#define WD_REG_INTVEC2      0x05

/* Read/Write Control Register */
#define WD_REG_CTL2		0x07
/* bits in control register */
#define WD_CTL_SRST     0x01    /* SSRST+ Reset the wdc. Minimum 25 miliseconds, logical or'ed with POR and PFD */
#define WD_CTL_LED      0x02	/* LED- */
#define WD_CTL_INTEN    0x04    /* INTEN+ Enable operation complete and bus error interrupts */
#define WD_CTL_SEQEN    0x08    /* SEQEN+ Enable DMA */
#define WD_CTL_INTEND0  0x10    /* INTEND0+ Enable Drive 0 completion Interrupt */
#define WD_CTL_INTEND1	0x20
#define WD_CTL_INTD0	0x40    /* INTD0+ Enable Drive 0 seek completion interrupt */
#define WD_CTL_INTD1	0x80


/* This address byte is written to by the host during I/O data transfer
   (Information transfer phase, host to controller) phase */
#define WD_HOST_WRITE		0x08

/* Status Read register */
#define WD_REG_STAT         0x09
/* status bits */
#define WD_STAT_BUSERR      0x01    /* + Bus error during wds's bus mastership */
#define WD_STAT_OUTEMPTY    0x02    /* + Output data register empty */
#define WD_STAT_OPCOMP      0x04    /* + Operation complete */
#define WD_STAT_INPFULL     0x08    /* + Input register full */
#define WD_STAT_SRESET      0x10    /* + SCSI bus in reset state */
#define WD_STAT_SMSG        0x20    /* + SCSI bus in message state */
#define WD_STAT_SBUSY       0x40    /* + SCSI bus busy */
#define WD_STAT_SCMD        0x80    /* + SCSI bus in command,status or message phase */

#define WD_REG_SELECT		0x0A
#define WD_REG_READINP		0x0B
#define WD_REG_CLRBUSERR	0x0C

#define SCSI_TESTREADY      0x00
#define SCSI_REZEROUNIT     0x01
#define SCSI_REQUESTSENSE   0x03
#define SCSI_FORMAT         0x04
#define SCSI_READ           0x08
#define SCSI_WRITE          0x0A
#define SCSI_SEEK           0x0B
#define SCSI_TRANSLATE      0x0F
#define SCSI_WRITEBUF       0x13
#define SCSI_READBUFRAM     0x14
#define SCSI_MODESELECT     0x15
#define SCSI_MODESENSE      0x1A
#define SCSI_STARTSTOP      0x1B
#define SCSI_RECDIAG        0x1C
#define SCSI_SENDDIAG       0x1D
#define SCSI_READCAPACITY   0x25
#define SCSI_READ2          0x28
#define SCSI_WRITE2         0x2A
#define SCSI_WRITEVERIFY    0x2E
#define SCSI_VERIFY         0x2F
#define SCSI_SEARCH         0x31

#define SENSE_NONE              0x00
#define SENSE_NO_INDEX          0x01
#define SENSE_NO_SEEK_COMPLETE  0x02
#define SENSE_WRITE_FAULT       0x03
#define SENSE_NOT_READY         0x04
#define SENSE_NO_TRACK0         0x06
#define SENSE_ID_CRC            0x10
#define SENSE_DATA              0x11
#define SENSE_ADDRESS_MARK      0x12
#define SENSE_DATA_ADDRESS_MARK 0x13
#define SENSE_RECORD_NOT_FOUND  0x14
#define SENSE_SEEK              0x15    /* Seek Error */
#define SENSE_DATA_CHECK_NR     0x18    /* Data Check in No Retry Mode */
#define SENSE_ECC_VERIFY        0x19    /* ECC Error During Verify */
#define SENSE_INTERLEAVE        0x1A    /* Interleave Error */
#define SENSE_UNFORMATTED       0x1C    /* Unformatted or Bad Format On Drive */
#define SENSE_SELFTEST          0x1D    /* Self Test Failed */
#define SENSE_INVALID_COMMAND   0x20
#define SENSE_BLOCK_ADDRESS     0x21    /* Illegal Block Address */
#define SENSE_VOLUME_OVERFLOW   0x23
#define SENSE_BAD_ARGUMENT      0x24
#define SENSE_LUN               0x25    /* Invalid Logical Unit Number */



/* for each unit = disk attached to one wd */
typedef struct {
	/* disk image backing store, added to make the drive real */
    FILE * img;
    char   imgName[FILENAME_MAX+1];
    UINT32 imgBlocks;         /* size of the image in 512 byte blocks */
    int    imgReadonly;
    int    cylinders;         /* set by mode select, required for formatting */
    int    sectors;
    int    heads;
    int    cylinder_rwc;
	int    cylinder_wpc;
    int    capacity;          /* blocks as in superblock (excluding diag and config record) */
} wd_unitRegs_t;

/* controller registers */
typedef struct {

    UINT32 dmaAddress;
    UINT8 intVector;
    //UINT8 intVectorError;
    UINT8 statusReg;
    UINT8 scsiCheck;
    UINT8 selected;
    UINT8 readInpReg;
    //UINT8 ctlReg;
    UINT8 hostWriteReg;
    UINT8 scsiBuf[WD_SCSICMD_MAX];
    UINT8 ctlReg2; /* 07 ?? */
    int scsiIdx;
    int installed;
    int state;
    int stateCounter;
    /* non scsi commands */
    int cmdCounter;
    int currCommand;
    UINT16 dmaTestWordRead;
    UINT8  dmaTestCount;    /* 2=HI(dmaTestWordRead),1=LO, 0=none */
    UINT32 intCount;
    int    intPending;     /* a completion interrupt is outstanding */
    int    intAsserted;    /* current state of the request line   */
    UINT32 replyBytesLeft;
    UINT32 replyBytePos;
    UINT8  replyBuffer[18*WD_SECTOR_SIZE];
    UINT8  sense[4];       /* sense bytes returned by REQUEST SENSE  */
    UINT8  statusByte;     /* SCSI status handed over in the status phase */
    int dataIdx;
    UINT8  dataBuf[WD_SECTOR_SIZE*8];
    /* --!! this has to be the last field in the struct and is not nulled on reset !!-- */
    wd_unitRegs_t units[WD_MAX_UNITS];
} wd_regs_t;

typedef enum {
    SCSI_S_IDLE,
    SCSI_S_MESSAGE,
    SCSI_S_BUSY,
    SCSI_S_COMPLETE,
    SCSI_S_READRESULTS,     /* data in phase,  status 0x48 */
    SCSI_S_STATUS,          /* status phase,   status 0xcc */
    SCSI_S_MESSAGEBYTE,     /* message phase,  status 0xe8 */
    SCSI_S_ENDOFCOMMAND
} SCSI_S;


typedef enum {
    CMD_RESET=0,
    CMD_SCSIRESET=3,
    CMD_RESET_OUTREGFULL,
    CMD_INFORMATION_TRANSFER,	// Information transfer phase when dma is off
    CMD_PROCESS_SCSICMD,
    //CMD_TRANSFER_PARAM_START,   // start of transferring data from hosts after command block in non dma mode
    CMD_SET_INPFULL
} CMD_S;


unsigned int wd_read_byte(unsigned int address, int flags);
unsigned int wd_read_word(unsigned int address, int flags);
void wd_write_byte(unsigned int address, unsigned int value, int flags);
void wd_write_word(unsigned int address, unsigned int value, int flags);
void wd_pulse_reset(void);
int wd_dbgCmd(int numArgs, struct args_t * args);
int wd_attach_image (int unit, int device, const char * name);
int wd_units_ready(void);
void wd_processContinue(void);  /* called each n instructions */
int  wd_irq_ack(int level);

int wd_save_state(FILE * f);
int wd_load_state(FILE * f);

#endif
