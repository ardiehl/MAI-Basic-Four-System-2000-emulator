#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "eagletest.h"
#include "linenoise.h"
#include "exprparse.h"
#include <inttypes.h>
#include <ctype.h>
#include "wd.h"

//#define INCLUDE_HEAPTEST



#define PROMPT "tests> "
#define PROMPTLENGTH 7


int numArgs;
struct args_t args [MAXNUMARGS];

void initSymbols() {
        exprparseAddSymbol("WD0",WD0_ADDR,"wd0 base register",0);
        exprparseAddSymbol("WD0dmaH",(uint32_t) wd0_dma_hi,"wd0 dma high",0);
        exprparseAddSymbol("WD0dmaM",(uint32_t) wd0_dma_mid,"wd0 dma mid",0);
        exprparseAddSymbol("WD0dmaL",(uint32_t) wd0_dma_lo,"wd0 dma low",0);
        exprparseAddSymbol("WD0i",(uint32_t) wd0_intvec,"wd0 interrupt vector register",0);
        //exprparseAddSymbol("WD0i2",(uint32_t) ,"?? Test reads from this port and requires value written to 0x04",0);
        exprparseAddSymbol("WD0c",(uint32_t) wd0_rwcontrol,"wd0 Read/Write Control Register",0);
        exprparseAddSymbol("WD0hw",(uint32_t) wd0_hostwrite,"wd0 Host Write to Output Register",0);
        exprparseAddSymbol("WD0stat",(uint32_t) wd0_status,"wd0 Status Register",0);
        exprparseAddSymbol("WD0sel",(uint32_t) wd0_select,"wd0 select",0);
        exprparseAddSymbol("WD0hr",(uint32_t) wd0_hostread,"wd0 Host Read Input Register",0);
        exprparseAddSymbol("WD0clr",(uint32_t) wd0_clearerr,"wd0 Host Clears Bus Error Latch",0);
}

char * statusNames[8] = {"MYBERR+","PIOINM+","OPCCMP+","PIOUTF+","SRESET+","MSG+","BUSY+","CMD+"};
char * statusFuncs[8] = {"Bus Error occurring during WDC's bus mastership","output Data Register empty","operation complete","input data register full","SCSI bus in reset phase","SCSI bus in message phase","SCSI bus busy","SCSI bus in commnand, status or message phase"};


void cmd_lstatus (int numArgs, struct args_t *args) {
	uint8_t status;
	if(numArgs) status=args[0].value; else status = *wd0_status;

	printf("status: %02x\n",status);
	printf("Bit Value name     function\n");
	for (int i=0;i<8;i++)
		printf(" %d  %3d   %-8s %s\n",i,status & (1<<i) ? 1 : 0,statusNames[i],statusFuncs[i]);
	printf("\n");
}


void cmd_status (int numArgs, struct args_t *args) {
	int s;

	if (numArgs == 0) {
		numArgs++;
		args[0].value = *wd0_status;
	}
	puts("   CMD BUSY MSG SRES OUTFULL COMPLETE INEMPTY BUSERR");
	for (int i=0;i<numArgs;i++) {
		s = args[i].value;
		printf("%02x   %d    %d   %d    %d       %d        %d       %d      %d\n",s,(s & 0x80) > 0,(s & 0x40) > 0,(s & 0x20) > 0,(s & 0x10) > 0,(s & 0x08) > 0,(s & 0x04) > 0,(s & 0x02) > 0,(s & 0x01) > 0);
	}
}


void invalidArgs(void) {
	printf("\ninvalid arguments\n");
}

void stringRequired(int arg) {
	printf("argument %d: string required\n",++arg);
}

void valueRequired(int arg) {
	printf("argument %d: value or value expression required\n",++arg);
}




void cmd_expr (int numArgs, struct args_t *args) {
	//printf("cmd_dummy %d args\n",numArgs);
	for (int i=0;i<numArgs;i++) {
		if (args[i].isValue)
			printf("#%i: value   #%d (%x)\n",i,args[i].value,args[i].value);
		else
		    printf("#%i: string '%s'\n",i,args[i].txt);
	}
}

void cmd_symlist (int numArgs, struct args_t *args) { expparseListSymbols (); }
void cmd_functions (int numArgs, struct args_t *args) { expparseListFunctions (); }


void cmd_symadd (int numArgs, struct args_t *args) {
	if (args[0].isValue) { stringRequired(0); return; }
	if (! args[1].isValue) { stringRequired(0); return; }
	exprparseAddSymbol (args[0].txt,args[1].value,"",1);
}


void cmd_symdel (int numArgs, struct args_t *args) {
	//if (args[0].isValue) { stringRequired(0); return; }
	if (exprparseDeleteSymbol (args[0].txt) < 0) printf("symbol not found\n");
}


void cmd_wwstat (int numArgs, struct args_t *args) {
	int timeout = 1000;
	if (numArgs > 3) timeout = args[3].value;
	//write_checkstatus (args[0].value,args[1].value,args[2].value,timeout);
	wd_writeCmdAndRecordStatus ((uint8_t *)args[0].value,args[1].value,args[2].value,timeout,"wstat");
}

void cmd_wreset (int numArgs, struct args_t *args) { wdc_reset(args[0].value); }

void cmd_wselect (int numArgs, struct args_t *args) {
	if (numArgs) wdc_select(args[0].value); else wdc_select(1);
}

void cmd_wsense (int numArgs, struct args_t *args) { wdc_sense(args[0].value); }
void cmd_wrezero  (int numArgs, struct args_t *args) { wdc_rezeroUnit(args[0].value); }
void cmd_wrstatus (int numArgs, struct args_t *args) { wd_printRecordedStatus(); }
void cmd_wrclear (int numArgs, struct args_t *args) { wdc_statValuesReset(); }

void cmd_help (int numArgs, struct args_t *args);

void cmd_whread (int numArgs, struct args_t *args) {
    uint8_t s1,s2,h;
    int count = args[0].value;
    if (!count) count++;
	//printf("count:%d\n",count);
	for (int i=0; i<count;i++) {
        s1 = *wd0_status;
        h = *wd0_hostread;
        s2 = *wd0_status;
        printf("s: %02x hr:%02x s:%02x\n",s1,h,s2);
	}
}

// for testing status after scsi command buffer send for differnt commands
void cmd_wwbytes (int numArgs, struct args_t *args) {
	int i;
	uint8_t cmdByte;
	int timeout = exprparseFindSymbol ("wwrtimeout", NULL);
	if (timeout < 1) timeout = 250;
	printf("timeout: %d\n",timeout);

	for (i=0;i<numArgs;i++) {
		cmdByte = args[i].value;
		printf("%02x ",cmdByte);
		wd_writeCmdAndRecordStatus (wd0_hostwrite, cmdByte, 0xff, timeout, NULL);
		printf("[%02x]  ",*wd0_status);
	}
	puts("");
}


struct cmds_t cmds[] =
{
	{ "expr"		, cmd_expr		, "iiiiiiiiii"	,"show result of an expression"		,""},
	{ "functions"	, cmd_functions	, ""    		,"show defined functions",""},
	{ "whread"		, cmd_whread  	, "i"   		,"read 1 or more bytes from host input reg","whread [num bytes]"},
	{ "wstatus"		, cmd_status	, "iiiiiiiiii"	,"show or decode status register value(s)",""},
	{ "wlstatus"	, cmd_lstatus	, "i"			,"show or decode status register or status value (detailed view)",""},
	{ "wrstatus"	, cmd_wrstatus	, ""			,"show wd recorded status"	,""},
	{ "wrclear"		, cmd_wrclear	, ""			,"clear recorded wd status", ""},
	{ "wreset"		, cmd_wreset	, "i"			,"wd reset"				,""},
	{ "wwbytes"		, cmd_wwbytes	, "Iiiiiiiiii"	,"write bytes to host write register and record status","<value> [value ...] timeout can be set via variable 'wwrtimeout'" },
	{ "wwstat"	 	, cmd_wwstat	, "IIIi"		,"write byte and record changes in status reg","<TargetAddress> <ValueToWrite> <expectedStatus> [timeout]"},
	{ "wsense"	 	, cmd_wsense	, "i"			,"wd sense"				,"[timeout] execute scsi sense and show result"},
	{ "wrezero"	 	, cmd_wrezero	, "i"			,"wd rezero"			,"[timeout] sets the selected drive to Track 0"},
	{ "wselect"  	, cmd_wselect	, "i"			,"wd select"			,"[0|1] select (1 or no arg) or deselect(0) contoller"},
	{ "symlist"  	, cmd_symlist	, ""    		,"show symbols"			,"List all defined symbols"},
	{ "symadd"   	, cmd_symadd	, "SI"  		,"add/change symbol"	,"<symbolname> <symbolvalue"},
	{ "symdel"   	, cmd_symdel	, "S"   		,"delete symbol"		,"<symbolname>"},

	{ "?"		 	, cmd_help		, "s"   		,"show this help or help for a command",""},
	{ "help"		, cmd_help		, "s"   		,"show this help or help for a command",""},
	{ "quit"		, NULL			, ""			,"terminate to internal debugger",""},
	{ NULL			, NULL			, "",""}
};



void showHelp (char * caption, struct cmds_t * cmds, int extended) {
	char st[255];
	int i;

	if (caption) {
		st[0]=0;
		for (i=0;i<strlen(caption);i++) strcat(st,"=");
		printf("\n %s\n %s\n",caption,st);
	}
	i=0;
	while (cmds[i].name) {
	    printf(" %-10s %s\n",cmds[i].name,cmds[i].usage);
		i++;
    }
	if (extended) {
		printf("args can be hex values or decimal values if started with #. Hex values need\n" \
		       "to start with 0..9, expressions including symbols and ()*/+-&| are supported\n");
	}
}



/* ========================================================================
 * Convert input string to number
 * returns 1 if value found
 * pure hex
 * #xx decumal number
 * ======================================================================== */

unsigned int strToULong(char * txt, unsigned int * value) {

	exprparse_t ex;

	exprparseInit (&ex, txt);
	if (exprparse (&ex)) {
		*value = ex.result;
		return 1;
	}
	return 0;

#if 0
	char buf[255];

	if (xtoui(txt,value)) return 1;  /* pure hex value */
	strcpy(buf,txt);
	if (buf[0]=='#') {  /* decimal value */
      *value = strtol(&buf[1],NULL,10);
	  return 1;
	}
  return 0;
#endif // 0
}

int findCommand (char * cmd) {
	int i = 0;
	int cmdNum = -1;
	int cmdPartMatchCount = 0;
	int j;
	int cmdNumPartMatch = -1;

	/* search command */
	//while ((cmdPartMatchCount < 2) && (cmdNum == -1) && (cmds[i].name)) {
	while ((cmdPartMatchCount < 2) && (cmdNum == -1) && (cmds[i].name)) {
		//printf("'%s'/'%s':%d/%d/%d ",cmd,cmds[i].name,strcmp(cmd,cmds[i].name),strlen(cmd),strlen(cmds[i].name));
	    if (strcmp(cmd,cmds[i].name) == 0) {
			return i;  // full match
		} else
		if ((j = strlen(cmd)) < strlen(cmds[i].name)) {
			if (strncmp(cmd,cmds[i].name,j) == 0) {
				cmdPartMatchCount++;
				cmdNumPartMatch = i;
			}
		}
		i++;
	}

	if (cmdPartMatchCount > 1) {
		printf("ambiguous command, candidates: ");
		i = 0;
		while (cmds[i].name) {
			if ((j = strlen(cmd)) < strlen(cmds[i].name)) {
				if (strncmp(cmd,cmds[i].name,j) == 0) {
					printf(cmds[i].name); putchar(' ');
				}
			}
			i++;
		}
		puts("");
		return -2;
	}
	if (cmdNum == -1) cmdNum = cmdNumPartMatch;
	return cmdNum;
}


void cmd_help (int numArgs, struct args_t *args) {
	if (numArgs) {
		int cmdNum = findCommand(args[0].txt);
		if (cmdNum > -1) {
			printf("%s\n%s %s\n",cmds[cmdNum].usage,cmds[cmdNum].name,cmds[cmdNum].detailedUsage);
		} else if (cmdNum != -2) printf("help: unknown command '%s'\n",args[0].txt);
	} else showHelp("Help for eagle tests",cmds,1);
}


int findAndExecCommand (	char * cmd,
                 			struct cmds_t *cmds, int numArgs,
                 			struct args_t *args) {
	int i;
	int cmdNum = -1;
	int res = 0;
	int minNumArgs=0;
	int maxNumArgs=0;
	char c;



	cmdNum = findCommand (cmd);
	if (cmdNum > -1) {
		maxNumArgs = strlen(cmds[cmdNum].argTypes);
		for (i=0;i<maxNumArgs;i++) {
			c = cmds[cmdNum].argTypes[i];
			if (isupper(c)) minNumArgs++;
			if (tolower(c) == 'i') {
				args[i].isValue = strToULong(args[i].txt,&args[i].value);
				if (args[i].isValue == 0) {
					printf("arg %d: value required\n",i);
					return res;
				}
			}
		}
		if (numArgs < minNumArgs) {
			printf ("at least %d args required\n",minNumArgs);
			return res;
		}
		if (numArgs > maxNumArgs) {
			printf ("max %d args allowed\n",minNumArgs);
			return res;
		}

		if (cmds[cmdNum].proc) {
			cmds[cmdNum].proc(numArgs,args);
			res = 1;
		}

	} else if (cmdNum > -2) printf("what? ('%s')\n",cmd);

	return res;
}

/* ========================================================================
 * command handler
 * Searches for command, checks required number of args, sets args and
 * calls routine given in cmds
 * If oneCmd is given, only that command will be executed
 * ======================================================================== */

char * lastCommand;  // to avoid duplicates in history

void commandHandler(char * oneCmd)
{
	char cmd[MAXCMDLEN+1];
	int i;
	char * tmp;
	char * token;

	do {
        for (i=0;i<MAXNUMARGS;i++) {	/* clear all args */
            args[i].txt[0] = 0;
            args[i].isValue = 0;
            args[i].value = 0;
        }
        if (oneCmd) {
            tmp = strdup(oneCmd);
            printf("%s %s\n",PROMPT, tmp);
        } else {
            tmp = linenoise(PROMPT);
            if (tmp)
                if (strlen(tmp) < 1) { free(tmp); tmp=NULL; }
        }
		if (tmp) {
			if (lastCommand) {
				if(strcmp(tmp,lastCommand) != 0) {
					linenoiseHistoryAdd (tmp);		/* add cmd line to histrory buffer */
					free(lastCommand);
					lastCommand = strdup(tmp);
				}
			} else linenoiseHistoryAdd (tmp);
			/* split command line into args */
			token = strtok(tmp," ");
			numArgs = 0;
			strncpy(cmd,token,sizeof(cmd)); cmd[sizeof(cmd)-1]=0;
			token = strtok(NULL," ");
			while ((token) && (numArgs < MAXNUMARGS)) {
				strncpy(args[numArgs].txt,token,sizeof(args[numArgs].txt));
				args[numArgs].txt[sizeof(args[numArgs].txt)-1] = 0;
				args[numArgs].isValue = 0; // strToULong(args[numArgs].txt,&args[numArgs].value);
				token = strtok(NULL," ");
				numArgs++;
			}
			free(tmp);
			findAndExecCommand (cmd,cmds,numArgs,args);
		}
		if (oneCmd) return;
	} while ((strcmp(cmd,"quit")));
}


#ifdef INCLUDE_HEAPTEST

#define NUMMALLOCS 1000
#define ALLOCSIZE 1024
void mtest() {
  char * m[NUMMALLOCS+1];
  int i,count;
/*
  printf("i@ %p\n",&i);
  m[0] = malloc(ALLOCSIZE);
  printf("malloc@ %p\n",m[0]);
  free(m[0]);
*/

  printf("i@ %p\n",&i);
  m[0] = malloc(ALLOCSIZE);
  printf("malloc@ %p\n",m[0]);
  free(m[0]);


  for (i=0;i<NUMMALLOCS;i++) m[i] = NULL;

  count=0;
  for (i=0;i<NUMMALLOCS;i++) {
	  m[i] = malloc(ALLOCSIZE);
	  if(m[i] == NULL) {
		  printf("malloc returned NULL\n");
		  break;
	  }
	  putchar('+'); count++;
	  //printf("malloc@ %08x\n",(unsigned int)m[i]);
	  memset (m[i], 0, ALLOCSIZE);
  }
  printf("\nAllocated %d times %d bytes\n",count,ALLOCSIZE);
  count=0;
  for (i=0;i<NUMMALLOCS;i++) {
	  if(m[i]) {
	    free(m[i]);
	    putchar('-');
	    m[i] = NULL;
	    count++;
      }
  }
  printf("\nfreed %d items\n",count);

}
#endif

#ifdef EAGLE
void unexpectedInt() {
	exit(0);  // drop to the internal debugger
}

void setVector(int vectorNum) {
	volatile uint32_t * vector = (uint32_t *) (vectorNum * 4);
	*vector = (uint32_t)&unexpectedInt;
}

void setVectors() {
	int i;
	for (i=0x18; i<=0x1f; i++) setVector(i);  // Spurious Interrupt, Level 1..7 Interrupt Autovector
	for (i=0x30; i<=0xff; i++) setVector(i);
}
#endif

int b_func(int argc, int args[]) {
	printf("func B, %d args ",argc);
	for (int i=0;i<argc;i++) printf(" %d:%d",i,args[i]);
	printf("\n");
	return 5;
}

/*
void test() {
	char i;

	printf("isdigit ");
	for (i='0'; i<'z'+1; i++) printf("(%c %d) ", i,isdigit(i));
	printf("\n");

	printf("isialpha ");
        for (i='0'; i<'z'+1; i++) printf("(%c %d) ", i,isalpha(i));
        printf("\n");

	printf("isxdigit ");
        for (i='0'; i<'z'+1; i++) printf("(%c %d) ", i,isxdigit(i));
        printf("\n");

	printf("strcasecmp(TeSt,tesT): %d\n",strcasecmp("TeSt","tesT"));

	printf("tolower ");
        for (i='0'; i<'z'+1; i++) printf("(%c %c) ", i,tolower(i));
        printf("\n");

	printf("toupper ");
        for (i='0'; i<'z'+1; i++) printf("(%c %c) ", i,toupper(i));
        printf("\n");

    char st[20]; char c;
    strcpy(&st[0],"tes");
    c='t';
    strncat(&st[0],&c,1);
    printf("%s\n",&st[0]);

}
*/

int main ()
{
#ifdef INCLUDE_HEAPTEST
  mtest();
#endif

#ifdef EAGLE
  setVectors();
#endif
  setbuf(stdout, NULL);
  //putchar('-'); putchar('*'); putchar('-');
  //printf("here we are\n\n");
  //printf("%s/%s/%d/%d\n\n","dummy",cmds[0].name,mystrcmp("dummy",cmds[0].name),strcmp("dummy",cmds[0].name));
  //printf("%s/%s/%d/%d\n\n","status",cmds[1].name,mystrcmp("status",cmds[1].name),strcmp("dummy",cmds[0].name));
  //mtest();
  //testReadStr();

  initSymbols ();
  exprparseAddFunction ("b",&b_func);
//test();
  //printf("Here we are\n");
  puts("eagletest v0.1 "__DATE__ " " __TIME__);
  commandHandler(NULL);
  return 0;

}
