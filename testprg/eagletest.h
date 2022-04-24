#ifndef TEST__HEADER
#define TEST__HEADER

void invalidArgs(void);

#define MAXCMDLEN 10
#define MAXNUMARGS 4
#define MAXARGLEN 30

struct args_t { 
	char txt[MAXARGLEN+1];
	int isValue;
	unsigned int value;
  };

struct cmds_t {
	char name[MAXCMDLEN+1];
	void  (*proc)(int numArgs, struct args_t *args);
	int  minNumArgs;
	int  maxNumArgs;
	int  requiresAllArgsHex;
	char * usage;
};

#endif
