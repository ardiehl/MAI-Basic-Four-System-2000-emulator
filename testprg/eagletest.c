#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "eagletest.h"

//void __reboot () {
  // here we need to boot
//}


int numArgs;

struct args_t args [MAXNUMARGS];


void invalidArgs(void) {
	printf("\ninvalid arguments\n");
}



void cmd_dummy (int numArgs, struct args_t *args) {
	printf("cmd_dummy %d args\n",numArgs);
}


void cmd_help (int numArgs, struct args_t *args);

struct cmds_t cmds[] =
{
    { "dummy", cmd_dummy    , 0,1,0,"dummy command"},
    { "?",     cmd_help     , 0,0,0,"show this help"},
	{ "help",  cmd_help     , 0,0,0,"show this help"},
	{ "quit", NULL          , 0,0,0,"terminate to internal debugger"},
    { "",  NULL, 0,0,0,""}
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
	while (cmds[i].name[0] != 0) {
	    printf(" %-10s %s\n",cmds[i].name,cmds[i].usage);
		i++;
    }
	if (extended) {
	}
}

void cmd_help (int numArgs, struct args_t *args) {

	showHelp("Help for eagle tests",cmds,1);
}


/* ========================================================================
 * Convert input string to number
 * returns 1 if value found
 * pure hex
 * #xx decumal number
 * ======================================================================== */

unsigned int strToULong(char * txt, unsigned int * value) {
	char buf[255];

	if (xtoui(txt,value)) return 1;  /* pure hex value */
	strcpy(buf,txt);
	if (buf[0]=='#') {  /* decimal value */	
      *value = strtol(&buf[1],NULL,10);
	  return 1;
	}
  return 0;
}

int findAndExecCommand (	char * cmd, 
                 			struct cmds_t *cmds, int numArgs, 
                 			struct args_t *args) {
	int i;
	int cmdNumPartMatch = -1; 
	int cmdPartMatchCount = 0;
	int cmdNum = -1;
	int allArgsAreHex = 1;
	char atleast[12];
	int j;
	int res = 0;
					

	for (i=0;i<numArgs;i++) {
		if (!(args[i].isValue)) allArgsAreHex = 0; 
	}
	i = 0;			 
	/* search command */
	while ((cmdPartMatchCount < 2) && (cmdNum == -1) & (cmds[i].name[0] != 0)) {
	    if (strcmp(cmd,cmds[i].name) == 0) {
			cmdNum = i;
		} else
		if ((j = strlen(cmd)) < strlen(cmds[i].name)) {
			if (strncmp(cmd,cmds[i].name,j) == 0) {
				cmdPartMatchCount++;
				cmdNumPartMatch = i;
			}
		}
		i++;
	}

	if ((cmdNum == -1) && (cmdPartMatchCount > 1)) {
		printf("ambiguous command\n");
	} else if (cmdNum != 0xffff) {
		if (cmdNum == -1) cmdNum = cmdNumPartMatch;
		if (cmdNum >= 0) {
			if (cmds[cmdNum].minNumArgs < cmds[cmdNum].maxNumArgs)
				strcpy(atleast,"at least ");
			else
				atleast[0]=0;
			if (numArgs < cmds[cmdNum].minNumArgs) {
				if (cmds[cmdNum].minNumArgs == 1) 
					printf("%s requires %sone arg\n",cmds[cmdNum].name,atleast);
				else
				    printf("%s requires %s%d args\n",cmds[cmdNum].name,atleast,cmds[cmdNum].minNumArgs);
			} else {
				if ((cmds[cmdNum].requiresAllArgsHex) && (allArgsAreHex == 0)) {
					printf("%s requires all args to be values\n",cmds[cmdNum].name);
				} else {
					if (cmds[cmdNum].proc) {
						cmds[cmdNum].proc(numArgs,args);
						res = 1;
					}
				}
			}
		} else printf("what?\n");
	}
	return res;
}

/* ========================================================================
 * command handler
 * Searches for command, checks required number of args, sets args and
 * calls routine given in cmds
 * If oneCmd is given, only that command will be executed
 * ======================================================================== */

void commandHandler(char * oneCmd)
{
	char cmd[MAXCMDLEN+1];
	int i,cmdNum;
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
		  printf("<tests> %s\n",tmp);
	  } else {
      	  tmp = readString("<tests>");		/* get command line */
	  	  if (tmp)
		    if (strlen(tmp) < 1) { free(tmp); tmp=NULL; }
	  }
	  if (tmp) {
		  //add_history (tmp);		/* add cmd line to histrory buffer */
		  /* split command line into args */
		  token = strtok(tmp," ");
		  numArgs = 0;
		  strncpy(cmd,token,sizeof(cmd)); cmd[sizeof(cmd)-1]=0;
		  token = strtok(NULL," ");
		  while ((token) && (numArgs < MAXNUMARGS)) {
			  strncpy(args[numArgs].txt,token,sizeof(args[numArgs].txt));
			  args[numArgs].txt[sizeof(args[numArgs].txt)-1] = 0;
			  args[numArgs].isValue = strToULong(args[numArgs].txt,&args[numArgs].value);
			  token = strtok(NULL," ");
			  numArgs++;
		  }
		  free(tmp);

		  cmdNum = -1;
		  /* check for register */
		  /*
		  if (strlen(cmd) == 2) {
			  if ((toupper(cmd[0]) == 'D') && (cmd[1] >= '0') && (cmd[1] <= '7')) {
				  numArgs = 1;
				  strcpy(args[0].txt,cmd);
				  dbgCmd_andn('D',M68K_REG_D0,numArgs,args);
				  cmdNum = 0xffff;
			  } else
			  if ((toupper(cmd[0]) == 'A') && (cmd[1] >= '0') && (cmd[1] <= '7')) {
				  numArgs = 1;
				  strcpy(args[0].txt,cmd);
				  dbgCmd_andn('A',M68K_REG_A0,numArgs,args);
				  cmdNum = 0xffff;
			  }  
		  }
         */
		if (cmdNum == -1) findAndExecCommand (cmd,cmds,numArgs,args);
	  }
	  /*printf("\n");*/
	if (oneCmd) return;
	} while ((strcmp(cmd,"quit")));
}



#define NUMMALLOCS 1000
#define ALLOCSIZE 1024
void mtest() {
  char * m[NUMMALLOCS+1];
  int i;
  
  for (i=0;i<NUMMALLOCS;i++) m[i] = NULL;
  
  for (i=0;i<NUMMALLOCS;i++) {
	  m[i] = malloc(ALLOCSIZE);
	  if(m[i] == NULL) {
		  printf("malloc returned NULL\n");
		  break;
	  }
	  putchar('+');
	  memset (m[i], 0, ALLOCSIZE);
  }
  printf("\nAllocated %d times %d bytes\n",i,ALLOCSIZE);
  for (i=0;i<NUMMALLOCS;i++) {
	  if(m[i]) {
	    free(m[i]);
	    putchar('-');
	    m[i] = NULL;
      }
  }
  printf("\nfreed %d items\n",NUMMALLOCS);
  
}

void testReadStr() {
	char * s;
	
	s = readString ("prompt>");
	printf ("got '%s', len=%d\n",s,(int)strlen(s));
	free(s);
}

int main ()
{
  setbuf(stdout, NULL);
  putchar('-'); putchar('*'); putchar('-');
  printf("here we are\n\n");
  
  mtest();
  //testReadStr();
  commandHandler(NULL);
  return 0;
  
  char c;
  do {

	  c = getch();
	  putchar(c);

  } while (c != ' ');
}
