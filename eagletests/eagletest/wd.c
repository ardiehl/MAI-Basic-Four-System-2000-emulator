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
#define MAX_STATUS_COMMANDS 256

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
	if (statValuesCount < MAX_STATUS_COMMANDS-1) statValuesCount++;
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
			//printf("Status %2x exp %2x timeout %d\n",status,expectedStatus,timeout);
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
	char valueStr[10];
	puts("## RW     address preStatus Exp Count*status ...");
	puts("----------------------------------------------------------------");
	for (i=0;i<statValuesCount;i++) {
		//if (statValues[i].rw == 'W') sprintf(valueStr,"%02x",statValues[i].commandByte);
		//else strcpy(valueStr,"  ");
		sprintf(valueStr,"%02x",statValues[i].commandByte);
		printf("%2d  %c %s%10p       %02x  %02x",i,statValues[i].rw,valueStr,statValues[i].address,statValues[i].initialStatus, statValues[i].expectedStatus);
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

int wdc_write (uint8_t expectedStatus, wd_command_t * w, int timeout, uint8_t expectedStatusLast, char *s) {
	int i;

	if (! wdc_expectedStatus(s, expectedStatus)) return 0;
	wdc_statValuesReset();
	for (i=0;i<w->byteCount;i++) {
		if(i == (w->byteCount-1)) expectedStatus = expectedStatusLast; // 0x48;
		if (!wd_writeCmdAndRecordStatus (wd0_hostwrite, w->data[i], expectedStatus, timeout, s)) {
			return 1;
		}
	}
	return 0;
}

int wdc_writeCommand (wd_command_t * w, int timeout, uint8_t expectedStatusLast) {
	int class = (w->data[0] >> 5) & 0x07;
	int len;
    switch (class) {
        case 0: len = 6; break;
        case 1: len = 10; break;
        default: printf("wdc_writeCommand: invalid scsi command class %d (cmd=%02x)",class,w->data[0]);
                 return 1;
    }
    if (len != w->byteCount) {
    	printf("wdc_writeCommand: invalid command block length %d, expected %d\n",w->byteCount,len);
		return 1;
    }
	return wdc_write(0xc2,w,timeout,expectedStatusLast,"writeCommand");
}

int wdc_writeData (wd_command_t * w, int timeout, uint8_t expectedStatusLast) {
	return wdc_write(0x42,w,timeout,expectedStatusLast,"writeData");
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
	"DEFECTIVE TRACK",				// 1E
	NULL,
	"INVALID COMMAND",              // 20
	"INV BLOCK ADDRESS",
	NULL,
	"VOLUME OVERFLOW",
	"BAD ARGUMENT",
	"INVALID LUN"                  // 25
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
		if ((errcode >= 0x10) && (errcode <= 0x25)) errtxt = wdc_errtxt10_1F[errcode-0x10];
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

void printHexByte (uint8_t b) {
	printf("%02x ",b);
}

void wdc_modesense (int timeout) {
	wd_command_t t;
	int res,i;

	// command block
	wdc_cmdInit (&t);
	wdc_cmdAddByte  (&t,0x1a);	// modelsense
	wdc_cmdAddByte (&t,0);      // upper 3 bit = lun
	wdc_cmdAddBytes (&t,0,2);	// 2 bytes reserved
	wdc_cmdAddByte (&t,22);     // number of bytes
	wdc_cmdAddByte (&t,0);
	res = wdc_writeCommand (&t,timeout,0x48); // [SBUSY+ INPFULL+]
	wdc_cmdFree(&t);
	if (res) {
		puts("wdc_writeCommand failed");
		return;
	}

	if (wdc_cmdAlloc (&t, 22) != 0) {
		wdc_failAlloc(22);
		wdc_cmdFree(&t);
		return;
	}

	if (wdc_receiveBytes (&t, 22, timeout, 0x48) > 0) {
		puts("wdc_receiveBytes failed");

	} else {
		for (i=0; i<22; i++) {
			printHexByte (t.receiveData[i]);
			if (i==3) puts("");
			if (i==11) puts("");
		}
		puts("");
		int pos = 0;
		printf("3 bytes reserved %02x %02x %02x\n",t.receiveData[pos],t.receiveData[pos+1],t.receiveData[pos+2]);
		pos += 3;
		printf("8 %02x\n",t.receiveData[pos]); pos++;
		printf("0 %02x\n",t.receiveData[pos]); pos++;
		printf("4 bytes reserved %02x %02x %02x %02x\n",t.receiveData[pos],t.receiveData[pos+1],t.receiveData[pos+2],t.receiveData[pos+3]);
		pos += 4;
		// 3 bytes blocksize
		uint32_t temp32 = (((uint32_t) t.receiveData[pos]) << 16) | (((uint32_t) t.receiveData[pos+1]) << 8) | t.receiveData[pos+2];

		printf("BlockSize: %lu\n",temp32);
		pos += 3;

		printf("1 %02x\n",t.receiveData[pos]); pos++;
		// 2 byte cylinder count
		temp32 = (((uint32_t) t.receiveData[pos]) << 8) | t.receiveData[pos+1];
		printf("Cylinders: %lu\n",temp32);
		pos += 2;

		printf("heads %d\n",t.receiveData[pos]); pos++;

		// 2 byte rwc
		temp32 = (((uint32_t) t.receiveData[pos]) << 8) | t.receiveData[pos+1];
		printf("rwc: %lu\n",temp32);
		pos += 2;

		// 2 byte wpc
		temp32 = (((uint32_t) t.receiveData[pos]) << 8) | t.receiveData[pos+1];
		printf("wpc: %lu\n",temp32);
		pos += 2;
		printf("landing zone pos %02x\n",t.receiveData[pos]); pos++;
		printf("Step pulse %02x\n",t.receiveData[pos]);


	}


}

void wdc_modesel (int timeout, int cylinders, int heads, int rwc, int steprate) {
	wd_command_t t;
	int res;

	// command block
	wdc_cmdInit (&t);
	wdc_cmdAddByte  (&t,0x15);	// modelsel
	wdc_cmdAddByte (&t,0);      // upper 3 bit = lun
	wdc_cmdAddBytes (&t,0,2);	// 2 bytes reserved
	wdc_cmdAddByte (&t,22);     // number of bytes
	wdc_cmdAddByte (&t,0);
	res = wdc_writeCommand (&t,timeout,0x40);
	wdc_cmdFree(&t);
	if (res) {
		puts("wdc_writeCommand failed");
		return;
	}

	// parameter
	wdc_cmdInit (&t);
	wdc_cmdAddBytes (&t,0,3);   // 3 x reserved
	wdc_cmdAddByte  (&t,0x08);	// length of extend descriptor list

	wdc_cmdAddBytes (&t,0,6);	// Densitiy code + 4x reserved + block size MSB
	wdc_cmdAddByte  (&t,0x02);	// block size 512
	wdc_cmdAddByte  (&t,0x00);	// block size 512

	wdc_cmdAddByte  (&t,0x01);	// List format code
	wdc_cmdAddByte  (&t,cylinders << 8);	// Cylinder count msb
	wdc_cmdAddByte  (&t,cylinders & 0x0f);	// Cylinder count lsb
	wdc_cmdAddByte  (&t,heads);
	wdc_cmdAddByte  (&t,rwc >> 8);		// reduced write current Cylinder msb
	wdc_cmdAddByte  (&t,rwc & 0x0f);	// reduced write current Cylinder lsb

	wdc_cmdAddBytes (&t,0,3);	// write precomp: ignored by controller, landing zone position
	wdc_cmdAddByte  (&t,steprate);

	res = wdc_writeData (&t,timeout,0xcc);
	wdc_cmdFree(&t);
	if (res) {
		puts("wdc_writeData failed");
		return;
	}

	res = *wd0_hostread;
	if (res != 0) {
		wdc_select(1);
		wdc_sense(timeout);
	}
}
