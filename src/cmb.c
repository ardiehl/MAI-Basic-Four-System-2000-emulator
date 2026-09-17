/***************************************************************************
 *   cmb.c
 *
 *  Tue November 22 21:23:11 2011
 *  Copyright  2011  Armin Diehl
 *  <ad@ardiehl.de>
 ****************************************************************************/
/*
 * mai 2000 cmb ports
 * see M8097 'MAI 2000 Desktop Computer System Sevice Manual'
 */

#include <stdio.h>
#include "m68k.h"
#include "sim.h"
#include "cmb.h"

int cmb_status_word;
unsigned int cmb_writeRegs;

/* adress must be a valid cmb port (2xxxxy) */
/* The parity error address registers and the general status register were
 * recognised but fell through to the unhandled path below, which returns all
 * ones. Per table 3-1 of BFISD 8079 the CMB status bits include the memory
 * management error flag, the main memory parity error flag, the power fail flag
 * and the stack overflow flag, so all ones reads as every fault on the board
 * being active at once. The boot PROM never looks, the BOSS/IX kernel does, and
 * it panicked with a memory management trap the moment it did. Nothing here is
 * latched by this emulator yet, so the honest answer is zero, no fault.
 */
unsigned int cmb_read_byte(unsigned int address) {
  switch (address & CMB_ADDR_REG_MASK) {
	  case CMBW_TSTOL		: { MSG (MSGC_ERR,MSG_CMB,MSG_READB,"attempt to read write only port TSTOL"); break; }
	  case CMBR_MEMPAR_HI	: { MSG (MSGC_INFO,MSG_CMB,MSG_READB,"read CMB_MEMPAR_HI, no fault latched"); return 0; }
	  case CMBR_MEMPAR_HI+1	: { MSG (MSGC_INFO,MSG_CMB,MSG_READB,"read CMB_MEMPAR_HI low byte, no fault latched"); return 0; }
	  case CMBR_MEMPAR_LO	: { MSG (MSGC_INFO,MSG_CMB,MSG_READB,"read CMB_MEMPAR_LO, no fault latched"); return 0; }
	  case CMBR_MEMPAR_LO+1	: { MSG (MSGC_INFO,MSG_CMB,MSG_READB,"read CMB_MEMPAR_LO low byte, no fault latched"); return 0; }
	  case CMBW_PARDATA		: { MSG (MSGC_ERR,MSG_CMB,MSG_READB,"attempt to read write only port PARDATA"); break; }
	  case CMBW_INHSER		: { MSG (MSGC_ERR,MSG_CMB,MSG_READB,"attempt to read write only port INHSER"); break; }
	  case CMBR_STATUS		: { MSG (MSGC_INFO,MSG_CMB,MSG_READB,"read STATUS8: %02x",cmb_status_word); return cmb_status_word & 0xff; }
	  case CMBR_STATUS+1	: { MSG (MSGC_INFO,MSG_CMB,MSG_READB,"read STATUS8 low byte: %02x",cmb_status_word & 0xff); return cmb_status_word & 0xff; }
	  case CMBR_GENSTATUS	: { MSG (MSGC_INFO,MSG_CMB,MSG_READB,"read GENSTATUS, no fault latched"); return 0; }
	  case CMBR_GENSTATUS+1	: { MSG (MSGC_INFO,MSG_CMB,MSG_READB,"read GENSTATUS low byte, no fault latched"); return 0; }
  }
  MSG (MSGC_ERR,MSG_CMB,MSG_READB,"unhandled read byte from address %08",address);
  return 0xff;
}

unsigned int cmb_read_word(unsigned int address) {
  switch (address & CMB_ADDR_REG_MASK) {
	  case CMBW_TSTOL		: { MSG (MSGC_ERR,MSG_CMB,MSG_READW,"attempt to read write only port TSTOL"); break; }
	  case CMBR_MEMPAR_HI	: { MSG (MSGC_INFO,MSG_CMB,MSG_READW,"read CMB_MEMPAR_HI, no fault latched"); return 0; }
	  case CMBR_MEMPAR_LO	: { MSG (MSGC_INFO,MSG_CMB,MSG_READW,"read CMB_MEMPAR_LO, no fault latched"); return 0; }
	  case CMBW_PARDATA		: { MSG (MSGC_ERR,MSG_CMB,MSG_READW,"attempt to read write only port PARDATA"); break; }
	  case CMBW_INHSER		: { MSG (MSGC_ERR,MSG_CMB,MSG_READW,"attempt to read write only port INHSER"); break; }
	  case CMBR_STATUS		: { MSG (MSGC_INFO,MSG_CMB,MSG_READW,"read STATUS16: %04x",cmb_status_word); return cmb_status_word; }
	  case CMBR_GENSTATUS	: { MSG (MSGC_INFO,MSG_CMB,MSG_READW,"read GENSTATUS, no fault latched"); return 0; }
  }
  MSG (MSGC_ERR,MSG_CMB,MSG_READW,"unhandled read word from address %08",address);
  return 0xffff;
}

unsigned int cmb_read_long(unsigned int address) {
  switch (address & CMB_ADDR_REG_MASK) {
	  case CMBW_TSTOL		: { MSG (MSGC_ERR,MSG_CMB,MSG_READL,"attempt to read write only port TSTOL"); break; }
	  case CMBR_MEMPAR_HI	: { MSG (MSGC_INFO,MSG_CMB,MSG_READL,"read CMB_MEMPAR_HI"); break; }
	  case CMBR_MEMPAR_LO	: { MSG (MSGC_INFO,MSG_CMB,MSG_READL,"read CMB_MEMPAR_LO"); break; }
	  case CMBW_PARDATA		: { MSG (MSGC_ERR,MSG_CMB,MSG_READL,"attempt to read write only port PARDATA"); break; }
	  case CMBW_INHSER		: { MSG (MSGC_ERR,MSG_CMB,MSG_READL,"attempt to read write only port INHSER"); break; }
	  case CMBR_STATUS		: { MSG (MSGC_INFO,MSG_CMB,MSG_READL,"read STATUS16: %04x",cmb_status_word); return cmb_status_word; }
	  case CMBR_GENSTATUS	: { MSG (MSGC_INFO,MSG_CMB,MSG_READL,"read GENSTATUS"); break; }
  }
  MSG (MSGC_ERR,MSG_CMB,MSG_READL,"unhandled read long from address %08",address);
  return 0xffffffff;
}

void cmb_setWriteRegs (int bit, int value) {
	int bitMask = 1 << bit;
	if (value) cmb_writeRegs |= bitMask;
	else cmb_writeRegs &= ~bitMask;
}


void cmb_write_byte(unsigned int address, unsigned int value) {
	switch (address & CMB_ADDR_REG_MASK) {
		case (CMBW_LED)		: { cmb_setWriteRegs (CMBWSTAT_LED,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEB,"LED: %d",value & 1); return; }
		case (CMBW_INHSER)	: { cmb_setWriteRegs (CMBWSTAT_INHSER,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEB,"SerialDrivers: %d",value & 1); return; }
		case (CMBW_INHPAR)	: { cmb_setWriteRegs (CMBWSTAT_INHPAR,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEB,"ParallelDrivers: %d",value & 1); return; }
		case (CMBW_TSTOL)	: { cmb_setWriteRegs (CMBWSTAT_TSTOL,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEB,"TestPoint TSTOL: %d",value & 1); return; }
		case (CMBW_TSTERL)	: { cmb_setWriteRegs (CMBWSTAT_TSTERL,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEB,"TestPoint TSERL: %d",value & 1); return; }
		case (CMBW_PARDATA)	: { cmb_setWriteRegs (CMBWSTAT_PARDATA,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEB,"Parity data forced: %d",value & 1); return; }
		case (CMBW_INHPARITY):{ cmb_setWriteRegs (CMBWSTAT_INHPARITY,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEB,"Inhibit parity response: %d",value & 1); return; }
	}
	MSG (MSGC_ERR,MSG_CMB,MSG_WRITEB,"unhandled write byte of %02x to address %08x",value,address);
}


void cmb_write_word(unsigned int address, unsigned int value) {
	switch (address & CMB_ADDR_REG_MASK) {
		case (CMBW_LED)		: { cmb_setWriteRegs (CMBWSTAT_LED,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEW,"LED: %d",value & 1); return; }
		case (CMBW_INHSER)	: { cmb_setWriteRegs (CMBWSTAT_INHSER,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEW,"SerialDrivers: %d",value & 1); return; }
		case (CMBW_INHPAR)	: { cmb_setWriteRegs (CMBWSTAT_INHPAR,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEW,"ParallelDrivers: %d",value & 1); return; }
		case (CMBW_TSTOL)	: { cmb_setWriteRegs (CMBWSTAT_TSTOL,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEW,"TestPoint TSTOL: %d",value & 1); return; }
		case (CMBW_TSTERL)	: { cmb_setWriteRegs (CMBWSTAT_TSTERL,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEW,"TestPoint TSERL: %d",value & 1); return; }
		case (CMBW_PARDATA)	: { cmb_setWriteRegs (CMBWSTAT_PARDATA,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEW,"Parity data forced: %d",value & 1); return; }
		case (CMBW_INHPARITY):{ cmb_setWriteRegs (CMBWSTAT_INHPARITY,value); MSG (MSGC_INFO,MSG_CMB,MSG_WRITEW,"Inhibit parity response: %d",value & 1); return; }
	}
	MSG (MSGC_ERR,MSG_CMB,MSG_WRITEW,"unhandled write word of %04x to address %08x",value,address);
}


void cmb_write_long(unsigned int address, unsigned int value) {
	MSG (MSGC_ERR,MSG_CMB,MSG_WRITEL,"unhandled write long of %08x to address %08x",value,address);
}

void cmb_pulse_reset(void) {
	cmb_status_word = (1 << CMBR_STATUS_BIT_HI1) |
		 			  (1 << CMBR_STATUS_BIT_HI2);
	cmb_writeRegs = 0;
}


void cmb_showRegs (int numArgs, struct args_t *args) {
	printf("cmb status register (%08x): %04x\r\n",CMB_ADDR+CMBR_STATUS,cmb_status_word);
	printf(" TSTOL: %d TSTERL: %d INHPAR: %d PARDATA: %d\n",
	       (cmb_status_word & CMBWSTAT_TSTOL) ? 1 : 0,
	       (cmb_status_word & CMBWSTAT_TSTERL) ? 1 : 0,
	       (cmb_status_word & CMBWSTAT_INHPAR) ? 1 : 0,
	       (cmb_status_word & CMBWSTAT_PARDATA) ? 1 : 0);
	printf(" INHSER: %d LED: %d INHPARITY: %d\n",
	       (cmb_status_word & CMBWSTAT_INHSER) ? 1 : 0,
	       (cmb_status_word & CMBWSTAT_LED) ? 1 : 0,
	       (cmb_status_word & CMBWSTAT_INHPARITY) ? 1 : 0);
}

void cmb_help (int numArgs, struct args_t *args);

struct cmds_t cmbCmds[] =
{
	{ "registers",	cmb_showRegs, 0,0,0,"show cmb registers"},
	{ "?",			cmb_help	, 0,0,0,"show this help"},
	{ "help",		cmb_help	, 0,0,0,"show this help"},
    { "",  NULL, 0,0,0,""}
};

void cmb_help (int numArgs, struct args_t *args) {
	showHelp ("cmb commands",cmbCmds,0);
}

int cmb_dbgCmd(int numArgs, struct args_t * args) {
	return findAndExecCommand (args[0].txt,cmbCmds,numArgs-1,&args[1]);
}

unsigned int cmb_getStatusWord (void) {
	return cmb_status_word;
}

unsigned int cmb_getWriteRegs (void) {
	return cmb_writeRegs;
}
