#include "wd.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef EAGLE

uint8_t wd0_dma_hiDummy;
uint8_t wd0_dma_midDummy;
uint8_t wd0_dma_loDummy;
uint8_t wd0_intvecDummy;
uint8_t wd0_rwcontrolDummy;
uint8_t wd0_hostwriteDummy;
uint8_t wd0_statusDummy;
uint8_t wd0_selectDummy;
uint8_t wd0_hostreadDummy;
uint8_t wd0_clearerrDummy;

#endif // EAGLE


void wdc_failAlloc(int bytes) {
	printf ("wdc: failed to alloc %d bytes\n",bytes);
}


#define MAX_STATUS_COUNT 10
#define MAX_STATUS_COMMANDS 16

typedef struct {
	char rw;
	uint8_t count;
	uint8_t * address;
	uint8_t commandByte;
	uint8_t expectedStatus;
	uint8_t initialStatus;
	uint8_t statusValue[MAX_STATUS_COUNT];
	uint16_t statusValueCount[MAX_STATUS_COUNT];
} statusrecord_t;

statusrecord_t statValues[MAX_STATUS_COMMANDS];
uint8_t statValuesCount;

void wdc_statValuesReset() {
	statValuesCount=0;
	for (int i=0; i<MAX_STATUS_COMMANDS; i++) {
		statValues[i].count=0;
		memset(&statValues[i].statusValue,0,MAX_STATUS_COUNT);
		//memset(&statValues[i].statusValueCount,0,MAX_STATUS_COUNT);
	}
}

void wdc_statValuesNext() {
	if (statValuesCount < MAX_STATUS_COMMANDS) statValuesCount++;
}

uint8_t wd_writeCmdAndRecordStatus (uint8_t * address, uint8_t cmdByte, uint8_t expectedStatus, int timeout, char * msg) {
	if (! timeout) timeout = WDC_DEF_TIMEOUT;
	statValues[statValuesCount].rw='W';
	statValues[statValuesCount].commandByte = cmdByte;
	statValues[statValuesCount].expectedStatus = expectedStatus;
	statValues[statValuesCount].initialStatus = *wd0_status;
	statValues[statValuesCount].address = address;
	uint8_t * statusValuePtr = & statValues[statValuesCount].statusValue[0];
	uint16_t * statusValueCountPtr = & statValues[statValuesCount].statusValueCount[0];
	uint8_t count = 1;
	//printf("%p 0x%02x 0x%02x %d\n",address,cmdByte,expectedStatus,timeout);

	*statusValueCountPtr=1;
	*address = cmdByte;
	uint8_t status = *wd0_status;
	*statusValuePtr = status;
	if (status != expectedStatus)
		do {
			status = *wd0_status;
			if (status != *statusValuePtr) {
				if (count < MAX_STATUS_COUNT) {
					count++;
					statusValuePtr++;
					statusValueCountPtr++;
					*statusValueCountPtr=0;
				}
				*statusValuePtr = status;
			}
			if (*statusValueCountPtr < 0xFFFF) (*statusValueCountPtr)++;
			timeout--;
		} while ((status != expectedStatus) && (timeout));

	statValues[statValuesCount].count = count;
	wdc_statValuesNext();
	if (msg) {
		if (status != expectedStatus) printf("%s failed, expected status %02x, got %02x\n",msg,expectedStatus,status);
	}
	return (status == expectedStatus);
}


uint8_t readResultAndRecordStatus (uint8_t * address, uint8_t * result, uint8_t expectedStatus, int timeout, char * msg) {
	if (! timeout) timeout = WDC_DEF_TIMEOUT;
	statValues[statValuesCount].rw='R';
	statValues[statValuesCount].commandByte = 0;
	statValues[statValuesCount].expectedStatus = expectedStatus;
	statValues[statValuesCount].initialStatus = *wd0_status;
	statValues[statValuesCount].address = address;
	uint8_t * statusValuePtr = & statValues[statValuesCount].statusValue[0];
	uint16_t * statusValueCountPtr = & statValues[statValuesCount].statusValueCount[0];
	uint8_t count = 1;
	//printf("%p 0x%02x %d\n",address,expectedStatus,timeout);

	*statusValueCountPtr=1;
	uint8_t status = *wd0_status;
	*statusValuePtr = status;
	if (status != expectedStatus)
		do {
			status = *wd0_status;
			if (status != *statusValuePtr) {
				if (count < MAX_STATUS_COUNT) {
					count++;
					statusValuePtr++;
					statusValueCountPtr++;
					*statusValueCountPtr=0;
				}
				*statusValuePtr = status;
			}
			if (*statusValueCountPtr < 0xffff) (*statusValueCountPtr)++;
			timeout--;
	} while ((status != expectedStatus) && (timeout));

	statValues[statValuesCount].count = count;
	*result = *address;
	wdc_statValuesNext();
	if (msg) {
		if (status != expectedStatus) printf("%s failed, expected status %02x, got %02x\n",msg,expectedStatus,status);
	}
	return (status == expectedStatus);
}

/*
uint8_t wd_writeCmdAndRecordStatus (uint8_t * address, uint8_t cmdByte, uint8_t expectedStatus, int timeout, char * msg) {
	wdc_statValuesReset();
	return wd_writeCmdAndRecordStatus (address, cmdByte, expectedStatus, timeout, msg);
}*/

void wd_printRecordedStatus() {
	int i;
	puts("## RW    address preStatus Exp Count*status ...");
	puts("----------------------------------------------------------------");
	for (i=0;i<statValuesCount;i++) {
		printf("%2d  %c %10p        %02x  %02x",i,statValues[i].rw,statValues[i].address,statValues[i].initialStatus, statValues[i].expectedStatus);
		for (int j=0; j<statValues[i].count; j++) {
			printf(" #%d*%02x",statValues[i].statusValueCount[j],statValues[i].statusValue[j]);
		}
		puts("");
	}
}

volatile int delayCounter;
void delay() {
	delayCounter=4000;
	while(delayCounter) delayCounter--;
}

// on real 2000: #1*00 1*10
void wdc_reset(int timeout) {
	wdc_statValuesReset();
	wd_writeCmdAndRecordStatus (wd0_rwcontrol, 3,0x10,timeout, "reset" );
	delay();
	wd_writeCmdAndRecordStatus (wd0_rwcontrol, 0,0,timeout, "reset" );
}

// select on real 2000: #12*00 #4*40 #1*c0 #1*c2
uint8_t wdc_select(uint8_t sel) {  // 0=deselect, 1=select
	wdc_statValuesReset();
	int expectedStatus = 0xc2;

	sel &= 1;
	//if (sel)
	//expectedStatus = 0xc2;
	wd_writeCmdAndRecordStatus (wd0_select, sel,expectedStatus,0, "select");
	return (*wd0_status == expectedStatus);
}

void wdc_cmdInit (wd_command_t * w) {
	w->byteCount=0;
	w->receivedDataLen=0;
	w->receiveDataLen=0;
	w->receiveData=NULL;
}

void wdc_cmdFree (wd_command_t * w) {
	if (w->receiveData) free(w->receiveData);
	wdc_cmdInit(w);
}

int wdc_cmdAlloc (wd_command_t * w, int bytes) {
	if (w->receiveData) free (w->receiveData);
	w->receiveDataLen = bytes;
	w->receiveData = malloc (bytes);
	if (w->receiveData == NULL) return 1;
	w->receivedDataLen = 0;
	return 0;
}

void wdc_cmdAddByte (wd_command_t * w, uint8_t b) {
	if (w->byteCount < WD_MAX_COMMAND_BYTES) {
		w->data[w->byteCount] = b;
		w->byteCount++;
		return;
	}
	puts("wdc_cmdAddByte: overflow");
}

void wdc_cmdAddBytes (wd_command_t * w, uint8_t b, uint8_t count) {
	for (uint8_t i=0; i<count; i++) wdc_cmdAddByte(w,b);
}

void wdc_cmdAddBlock (wd_command_t * w, int blockNo) {
	if (w->data[0] < 0x20) {	// class 0 commands, 21 bit block addr
		if (w->byteCount != 2) {
			puts("wdc_cmdAddBlock: count!=2");
			return;
		}
		wdc_cmdAddByte (w,(blockNo & 0x0ff00) >> 8);
		wdc_cmdAddByte (w,blockNo & 0x0ff);
		w->data[1] = (w->data[1] & 0xE0) | ((blockNo & 0x1f0000)>>16);
	} else {
		// 32 bit block address
		wdc_cmdAddByte(w, (blockNo & 0x0ff000000) >> 24);
		wdc_cmdAddByte(w, (blockNo & 0x000ff0000) >> 16);
		wdc_cmdAddByte(w, (blockNo & 0x00000ff00) >> 8);
		wdc_cmdAddByte(w, blockNo & 0x0000000ff);
	}
}

int wdc_expectedStatus (char * msg, uint8_t status) {
	uint8_t st = *wd0_status;
	if (st == status) return 1;
	printf("%s: status %02x but expected %02x\n",msg, st, status);
	return 0;
}

int wdc_writeCommand (wd_command_t * w, int timeout, uint8_t expectedStatusLast) {
	int i;
	uint8_t expectedStatus = 0xc2;

	if (! wdc_expectedStatus("writeCommand", 0xc2)) return 0;
	wdc_statValuesReset();
	for (i=0;i<w->byteCount;i++) {
		if(i == (w->byteCount-1)) expectedStatus = expectedStatusLast; // 0x48;
		if (!wd_writeCmdAndRecordStatus (wd0_hostwrite, w->data[i], expectedStatus, timeout,"writeCommand")) {
			return 1;
		}
	}
	return 0;
}


int wdc_receiveBytes (wd_command_t * w, int numBytes, int timeout, uint8_t expectedStatus) {
	int i;
	char * result = w->receiveData;
	printf("wdc_receiveBytes len:%d\n",w->receiveDataLen);

	w->receivedDataLen = 0;

	for (i=0;i<w->receiveDataLen;i++) {
		if (! readResultAndRecordStatus (wd0_hostread, (uint8_t *) result, expectedStatus, timeout, "receiveBytes")) {
			//printf("readResultAndRecordStatus, expected %02x, got %02x\n",0x48,*wd0_status);
			return 1;
		}
		result++;
		w->receivedDataLen++;
	}
	return 0;
}

char * wdc_errtxt00_06[] = {
	"NO SENSE", // 00
	"NO INDEX SIGNAL",		// 01
	"NO SEEK COMPLETE",		// 02
	"WRITE FAULT",			// 03
	"DRIVE NOT READY",		// 04
	NULL,					// 05
	"NO TRACK ZERO",		// 06
};

char * wdc_errtxt10_1F[] = {
	"ID CRC ERROR",			// 0x10
	"UNCORRECTABLE DATA ERROR",
	"ID ADDRESS MARK NOr FOUND",
	"DATA ADDRESS MARK NOT FOUND",
	"RECORD NOT FOUND",
	"SEEK ERROR",
	NULL,
	NULL,
	"DATA CHECK IN NO RETRY IDDE",
	"ECC ERROR DURING VERIFY",		// 19
	"INTERLEAVE ERROR",				// 1A
	NULL,							// 1B
	"UNFORMATTED",					// 1C
	"SELF TEST FAILED",				// 1D
	"DEFECTIVE TRACK"				// 1E
};


void wdc_sense (int timeout) {
	wd_command_t t;
	uint8_t errcode;
	char null = 0;
	char * errtxt;
	int res;

	wdc_cmdInit (&t);
	wdc_cmdAddByte  (&t,3);	// sense
	wdc_cmdAddBytes (&t,0,3);
	wdc_cmdAddByte  (&t,4);	// result size
	wdc_cmdAddByte  (&t,0);

	res = wdc_writeCommand (&t,timeout,0x48);  // 0x48
	printf("res: %d\n",res);
	if (res > 0) {
		wdc_cmdFree(&t);
		return;
	}

	if (wdc_cmdAlloc (&t, 4) != 0) {
		wdc_failAlloc(4);
		wdc_cmdFree(&t);
	}

	if (wdc_receiveBytes (&t, 4, timeout, 0x48) > 0) {
		puts("wdc_receiveBytes failed");
	} else {
		errcode = (uint8_t) t.receiveData[0] & 0x7f;
		errtxt = NULL;
		if (errcode <= 6) errtxt = wdc_errtxt00_06[errcode]; else
		if ((errcode >= 0x10) && (errcode <= 0x1e)) errtxt = wdc_errtxt10_1F[errcode-0x10];
		if (errtxt == NULL) errtxt = &null;
		printf("Error code: 0x%02x %s\n",errcode,errtxt);

		errcode = (uint8_t) t.receiveData[0] & 0x80;
		if (errcode) {	// valid logical block address
			int blockAddr;
			int i;

			i = (uint8_t) t.receiveData[1] & 0x1f;
			blockAddr = i << 22;
			blockAddr |= t.receiveData[2] << 8;
			blockAddr |= t.receiveData[3];
			printf("Block addr: %d (0x%x)\n",blockAddr,blockAddr);
		}
	}

	wdc_cmdFree(&t);
}


void showScsiResult(uint8_t status) {
	printf("result code: %02x (BUSY:%d, EQUAL:%d, CHECK:%d)\n",status,((status & 0x08) >0),((status & 0x04) >0),((status & 0x20) >0));
}

void wdc_rezeroUnit (int timeout) {
	wd_command_t t;
	int res;

	wdc_cmdInit (&t);
	wdc_cmdAddByte  (&t,1);	// rezero
	wdc_cmdAddBytes (&t,0,5);

	res = wdc_writeCommand (&t,timeout,0xcc);
	printf("res: %d\n",res);
	if (res > 0) {
		wdc_cmdFree(&t);
		return;
	}

	if (wdc_cmdAlloc (&t, 1) != 0) {
		wdc_failAlloc(1);
		wdc_cmdFree(&t);
	}

	if (wdc_receiveBytes (&t, 1, timeout, 0xcc) > 0) {
		puts("wdc_receiveBytes failed");
	} else {
		showScsiResult(t.receiveData[0]);
	}

	wdc_cmdFree(&t);
}
