#ifndef TEST__HEADER
#define TEST__HEADER

void invalidArgs(void);

#define MAXCMDLEN 10
#define MAXNUMARGS 10
#define MAXARGLEN 30

struct args_t {
	char txt[MAXARGLEN+1];
	int isValue;
	unsigned int value;
  };

struct cmds_t {
	//char name[MAXCMDLEN+1];
	char * name;
	void  (*proc)(int numArgs, struct args_t *args);
	// types, upper case=required, lower case=optional
	// I=Integer, S=String
	char argTypes[MAXNUMARGS+1];
	//int  minNumArgs;
	//int  maxNumArgs;
	//int  requiresAllArgsHex;
	char * usage;
	char * detailedUsage;
};

#endif
