#ifndef WD_H_INCLUDED
#define WD_H_INCLUDED

#include <inttypes.h>

/*
WINCHESTER DISK CONTROLLER REGISTERS
Address        76543210        TYPE    Command
CX0000         DMA high        write   ldma
     1         DMA               .      .
     2         DMA               .      .
     3         DMA low           .      .
     4         VECTOR          write    vector
     5         VECTOR          read     rvector
     6         not used
     7         CONTROL          w/r     control
     8         DATA out        write    wdata
     9         STATUS          read     status
     A         SASI Select     write    select
     B         DATA in         read     rdata
     C         BERR clear      write    berrc

              STATUS REG.                             CONTROL REG.
  7   6   5   4   |   3   2   1   0         7   6   5   4   |   3   2   1   0
|C/D|BSY|MSG|RST  |  IDA|DUN|OMT|BER|                       |  DMA|INT|LED|CLR|

WDC ERROR CODES

CODE   MEANING             CODE   MEANING

00     No sense info.      15     Seek error
01     No index signal     18     Data check (no retry mode)
02     No seek complete    19     Verify ECC error
03     Write fault         1A     Interleave error
04     Unit not ready      1C     Bad disk format
06     No track 00         1D     Self test fail
10     I.D. CRC err.       1E     Bad track
11     Data error          20     Invalid cmd.
12     No I.D. Addr. mark  21     Illegal block addr.
13     No Data Addr. mark  23     Volume overflow
14     No record found     24     Bad command argument
                           25     Bad unit specified


*/

#define WD0_ADDR                0xCC0000
#define WD1_ADDR                0xCD0000



#ifdef EAGLE
#define wd0_dma_hi    (uint8_t *)(WD0_ADDR+1)
#define wd0_dma_mid   (uint8_t *)(WD0_ADDR+2)
#define wd0_dma_lo    (uint8_t *)(WD0_ADDR+3)
#define wd0_intvec    (uint8_t *)(WD0_ADDR+4)
#define wd0_rwcontrol (uint8_t *)(WD0_ADDR+7)
#define wd0_hostwrite (uint8_t *)(WD0_ADDR+8)
#define wd0_status    (uint8_t *)(WD0_ADDR+9)
#define wd0_select    (uint8_t *)(WD0_ADDR+0x0a)
#define wd0_hostread  (uint8_t *)(WD0_ADDR+0x0b)
#define wd0_clearerr  (uint8_t *)(WD0_ADDR+0x0c)
#else

extern uint8_t wd0_dma_hiDummy;
extern uint8_t wd0_dma_midDummy;
extern uint8_t wd0_dma_loDummy;
extern uint8_t wd0_intvecDummy;
extern uint8_t wd0_rwcontrolDummy;
extern uint8_t wd0_hostwriteDummy;
extern uint8_t wd0_statusDummy;
extern uint8_t wd0_selectDummy;
extern uint8_t wd0_hostreadDummy;
extern uint8_t wd0_clearerrDummy;

#define wd0_dma_hi    (& wd0_dma_hiDummy)
#define wd0_dma_mid   (& wd0_dma_midDummy)
#define wd0_dma_lo    (& wd0_dma_loDummy)
#define wd0_intvec    (& wd0_intvecDummy)
#define wd0_rwcontrol (& wd0_rwcontrolDummy)
#define wd0_hostwrite (& wd0_hostwriteDummy)
#define wd0_status    (& wd0_statusDummy)
#define wd0_select    (& wd0_selectDummy)
#define wd0_hostread  (& wd0_hostreadDummy)
#define wd0_clearerr  (& wd0_clearerrDummy)
#endif // EAGLE

#define WD_MAX_COMMAND_BYTES 64
#define WDC_DEF_TIMEOUT 4000

typedef struct {
	uint8_t byteCount;
	uint8_t data[WD_MAX_COMMAND_BYTES];
	int receivedDataLen;
	int receiveDataLen;
	char * receiveData;
} wd_command_t;

void wdc_cmdReset (wd_command_t * w);
void wdc_cmdAddByte (wd_command_t * w, uint8_t b);
void wdc_cmdAddBlock (wd_command_t * w, int blockNo);

void wd_printRecordedStatus();
void wdc_statValuesReset();  // clear recorded status data
uint8_t wd_writeCmdAndRecordStatus (uint8_t * address, uint8_t cmdByte, uint8_t expectedStatus, int timeout, char * msg);
void wdc_reset(int timeout);
uint8_t wdc_select(uint8_t sel);  // 0=deselect, 1=select
uint8_t wd_writeCmdAndRecordStatus (uint8_t * address, uint8_t cmdByte, uint8_t expectedStatus, int timeout, char * msg);

void wdc_sense (int timeout);
void wdc_rezeroUnit (int timeout);

#endif // WD_H_INCLUDED
