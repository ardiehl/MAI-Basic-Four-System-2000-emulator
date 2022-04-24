/*
 Simple integer parser supporting a sybol/value list

 +-
 ()
 & bitwise and
 | bitwise or
 ^ power

 e.g.
 1+2*3+(Basemem|4)^2

 Input number format depending on HEXDEFAULT
 0 numbers will be interpreted as decimal by default
   hex number need to start with 0x
 1 numbers are hex by default but needs to start with 0..9 (because of symbols)
   decimal numbers need to be started with #


*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "exprparse.h"
#include <string.h>

int term  (exprparse_t * ex);
int power (exprparse_t * ex);

#define EOFCHAR '\0'
#define MAXTOKEN 20

// if set to 1, default input is hex, decimal with prefix #
// if set to 0, default is decimal, hex with prefix 0x
#define HEXDEFAULT 1
#define DEBUG_MESSAGES 0

#define DBGPRINTF(V,...) if (DEBUG_MESSAGES>0) printf(V, __VA_ARGS__)
#define DBGPRINTF1(V) if (DEBUG_MESSAGES>0) printf(V)

symbol_t * symbols;

char * T_ALREADYDEFINED = " already defined\n";

int exprparseAddSymbol (char * name, int value, char * desc, int allocNameMemory) {
	symbol_t * s;
	if (! name) return 0;

	if (exprparseFindSymbol (name, &s) != -1) {
		s->value = value;
		//puts("Symbol "); puts(name); puts (T_ALREADYDEFINED);
		return 0;
	}

	if (symbols) {
		s = symbols;
		while (s->next) s = s->next;
		s->next=(symbol_t *) malloc(sizeof(symbol_t));
		if (! s-> next) return 0;
		s = s->next;
	} else {
		symbols = (symbol_t *) malloc(sizeof(symbol_t));
		s = symbols;
		if (! s) return 0;
	}
	s->name = name;
	s->value = value;
	s->next = NULL;
	s->desc = desc;
	s->nameAlloced = allocNameMemory;
	if (allocNameMemory) s->name = strdup(name);
	return 1;
}

/******************************************************************************/

int exprparseFindSymbol (char * name, symbol_t ** sym) {  // -1=not found else value
	symbol_t * s;

	if (! name) return -1;
	if (*name == 0) return -1;
	s = symbols;
	while (s) {
		if (strcasecmp(name,s->name) == 0) {
			DBGPRINTF("%d\t  SYMBOL %s\n", s->value, name);
			if (sym) *sym = s;
			return s->value;
		}
		s = s->next;
	}
	DBGPRINTF("%s\t  SYMBOL\n", "unknown");
	if (sym) *sym = NULL;
	return -1;
}

/******************************************************************************/

int exprparseDeleteSymbol (char * name) {  // -1=not found else 0
	symbol_t * s, * slast;

	if (! name) return -1;
	if (*name == 0) return -1;

	slast=NULL;
	s = symbols;
	while (s) {
		if (strcasecmp(name,s->name) == 0) {
			if (s->nameAlloced) free (s->name);
			if (s == symbols) {	// root
				symbols = s->next;
				free(s);
				return 0;
			} else {
				slast->next = s->next;
				free(s);
				return 0;
			}
		}
		slast = s;
		s = s->next;
	}
	return -1;
}

/******************************************************************************/

void expparseListSymbols () {
	symbol_t * s;
	char blank = '\0';
	char *desc;

	if (! symbols) return;

#if HEXDEFAULT == 1
	printf("Name                     Value Description\n");
	for (int i=0;i<78;i++) putchar('-');
	      //12345678901234567890 123456789
	putchar('\r'); putchar('\n');

	s = symbols;
	while (s) {
		desc = s->desc;
		if(!desc) desc = &blank;
		printf("%-20s %9x %s\n",s->name,s->value,desc);
		s = s->next;
	}
#else
	printf("Name                 Value dec Value hex\n");
	for (int i=0;i<78;i++) putchar('-');
	      //12345678901234567890 123456789 123456789
	putchar('\r'); putchar('\n');

	s = symbols;
	while (s) {
		desc = s->desc;
		if(!desc) desc = &blank;
		printf("%-20s %9d %9x %s\n",s->name,s->value,s->value,desc);
		s = s->next;
	}
#endif // HEXDEFAULT
}

/******************************************************************************/

func_t * functions;

func_t * exprparseFindFunction (char * name) {
	func_t * f = functions;

	while(f) {
	//printf("f: %s\n",f->functionName);
		if(strcmp(name,f->functionName) == 0) return f;
		f = (func_t *)f->next;
	}
	DBGPRINTF("%s\t  FUNCTION %s\n", "unknown", name);
	return NULL;
}


void exprparseAddFunction (char *name, funcPtr_t func) {
	func_t * f;

	if (exprparseFindFunction(name)) {
		puts("Function "); puts(name); puts (T_ALREADYDEFINED);
		return; /* already there */
	}
	if(!functions) {
		f = (func_t *)calloc(1,sizeof(func_t));
		functions = f;
	} else {
		f = functions;
		while (f->next) f = (func_t *)f->next;
		f->next = (func_t *)calloc(1,sizeof(func_t));
		f = (func_t *)f->next;
	}
	f->functionName = name;
	f->f = func;
	return;
}

/******************************************************************************/

void expparseListFunctions () {
	func_t * f;
	char blank = '\0';
	char *desc;

	if (! symbols) return;

	printf("Function            Description\n");
	for (int i=0;i<78;i++) putchar('-');
	      //12345678901234567890 123456789
	putchar('\r'); putchar('\n');

	f = functions;
	while (f) {
		desc = f->desc;
		if(!desc) desc = &blank;
		printf("%-20s %s\n",f->functionName,desc);
		f = f->next;
	}
}

/******************************************************************************/

char ex_getchar (exprparse_t * ex) {
	char c;

	if (ex->inputStr == NULL) return (EOFCHAR);
	c = *ex->nextChar;
	ex->currChar = c;
	if (c == 0) return (EOFCHAR);
	ex->currpos++;
	ex->nextChar++;
	return (c);
}


/******************************************************************************/

void ex_ungetchar (exprparse_t * ex) {
	if ((ex->currpos > 0) & (ex->currChar != EOFCHAR)) {
		//if (*ex->nextChar) {
			ex->currpos--;
			ex->nextChar--;
		//}
	}
}

/******************************************************************************/

void printerror (exprparse_t * ex, char * s1, char * s2) {
	printf("\r%s\n",ex->inputStr);
	for (int i=1; i<ex->currpos; i++) putchar(' ');
	printf("|--- ERROR: %s %s\n\n", s1,s2);
}

/******************************************************************************/

int isNameChar (char c) {
	c = tolower(c);
	if(isalpha(c)) return 1;
	if(c=='_') return 1;
	return 0;
}


void strccat(char *dest, char c) {
	while (*dest) dest++;
	*dest=c;
	dest++;
	*dest='\0';
}

void getToken(exprparse_t * ex) {
	char nameBuf[MAXTOKEN+1];
	int nameLen;

	while ((ex_getchar(ex)) == ' '); //Skip Blanks
	if(isNameChar(ex->currChar)) {
			memset(&nameBuf,0,sizeof(nameBuf)); nameLen = 0;
			while ((isNameChar(ex->currChar)) || (isdigit(ex->currChar))) {
				nameLen++;
				if(nameLen > MAXTOKEN) {
					ex->errpos = ex->currpos;
					ex->type = ERROR;
					return;
				}
				strccat(&nameBuf[0], ex->currChar);  // strncat(&nameBuf[0], (char *)&ex->currChar, 1);
				ex_getchar(ex);
			}
			ex_ungetchar(ex);
			DBGPRINTF("got name '%s'\n",nameBuf);
			ex->f = exprparseFindFunction (nameBuf);
			if (ex->f) {
				ex->type=FUNCT;
				DBGPRINTF("%s\tFUNCT\n",nameBuf);
				return;
			}
			ex->ival = exprparseFindSymbol ((char *)&nameBuf,NULL);
			if (ex->ival < 0) {
				ex->errpos = ex->currpos;
				ex->type = ERROR;
				printerror(ex,"symbol or function not found","");
				return;
			}
			ex->type=NUMBER;
			DBGPRINTF("%s\tSYMBOL\n",nameBuf);

	}
#if HEXDEFAULT == 1
	else if((isdigit(ex->currChar)) || (ex->currChar == '#')) {
		ex->type=NUMBER;
		ex->ival=0;
		if (ex->currChar != '#') {		// hex not starting with #
			ex->currChar = toupper(ex->currChar);
			while(isxdigit(ex->currChar)) {
				if(ex->currChar > '9') ex->currChar -= 7;
				ex->ival = 16*ex->ival + ex->currChar - '0';
				ex->currChar=toupper(ex_getchar(ex));
			}
		} else {
			ex->currChar = ex_getchar(ex);
			while(isdigit(ex->currChar)) {						// decimal without prefix
				ex->ival = 10*ex->ival + ex->currChar - '0';
				ex->currChar=ex_getchar(ex);
			}
		}
		ex_ungetchar(ex);
		DBGPRINTF("%d\tNUMBER \n", ex->ival);
		ex->type = NUMBER;
#else
	else if(isdigit(ex->currChar)) {
		ex->type=NUMBER;
		ex->ival=0;
		if ((*ex->nextChar == 'x') || (*ex->nextChar == 'X')) {		// hex starting with 0x ?
			ex_getchar(ex); ex->currChar = toupper(ex_getchar(ex));
			while(isxdigit(ex->currChar)) {
				if(ex->currChar > '9') ex->currChar -= 7;
				ex->ival = 16*ex->ival + ex->currChar - '0';
				ex->currChar=toupper(ex_getchar(ex));
			}
		} else {
			while(isdigit(ex->currChar)) {						// decimal wothout prefix
				ex->ival = 10*ex->ival + ex->currChar - '0';
				ex->currChar=ex_getchar(ex);
			}
		}
		ex_ungetchar(ex);
		DBGPRINTF("%d\tNUMBER \n", ex->ival);
		ex->type = NUMBER;
#endif
	} else {
		switch(ex->currChar){
		case ',':
			DBGPRINTF("%c\tCOMMA\n",ex->currChar);
			ex->type = COMMA;
			break;
		case '(':
			DBGPRINTF("%c\tLPAREN\n",ex->currChar);
			ex->type = LPAREN;
			break;
		case ')':
			DBGPRINTF("%c\tRPAREN\n",ex->currChar);
			ex->type = RPAREN;
			break;
		case '+':
			DBGPRINTF("%c\tPLUS\n",ex->currChar);
			ex->type = PLUS;
			break;
		case '-':
			DBGPRINTF("%c\tMINUS\n",ex->currChar);
			ex->type = MINUS;
			break;
		case '/':
			DBGPRINTF("%c\tDIVIDE\n",ex->currChar);
			ex->type = DIVIDE;
			break;
		case '*':
			DBGPRINTF("%c\tMULT\n",ex->currChar);
			ex->type = MULT;
			break;
		case '^':
			DBGPRINTF("%c\tPOWER\n",ex->currChar);
			ex->type = POWER;
			break;
		case '%':
			DBGPRINTF("%c\tREMAINDER\n",ex->currChar);
			ex->type = REMAINDER;
			break;
		case '&':
			DBGPRINTF("%c\tAND\n",ex->currChar);
			ex->type = AND;
			break;
		case '|':
			DBGPRINTF("%c\tOR\n",ex->currChar);
			ex->type = OR;
			break;
		case EOFCHAR:
			DBGPRINTF1("EOL\n");
			ex->type = EOL;
			break;
		default:
			ex->type=ERROR;
			DBGPRINTF("ERROR: %c\n", ex->currChar);
			ex->type = ERROR;
			break;
		}
	}
}

/******************************************************************************/

void getTokenName (char * name, TokenType tkType){
	switch(tkType){
		case FUNCT : sprintf(name,"FUNC"); break;
		case PLUS: sprintf(name,"+"); break;
		case MINUS: sprintf(name,"-"); break;
		case DIVIDE: sprintf(name,"/"); break;
		case MULT: sprintf(name,"*"); break;
		case REMAINDER: sprintf(name,"\%"); break;
		case POWER: sprintf(name,"^"); break;
		case LPAREN: sprintf(name,"("); break;
		case RPAREN: sprintf(name,")"); break;
		case NUMBER: sprintf(name,"NUMBER"); break;
		case AND: sprintf(name,"AND"); break;
		case OR: sprintf(name,"OR"); break;
		case EOL: sprintf(name,"EOL"); break;
		case COMMA: sprintf(name,","); break;
		default: sprintf(name,"??");
	}
	//printf("Token value: %d\n",ex->ival);
	//printf("Token curr char: %c\n\n",ex->currChar);
}


int match (exprparse_t * ex, TokenType tkType) {
char * exName[20];
//char * currName[20];

	if(ex->type == tkType){
		getToken(ex);
		return 1;
	} else {
		if (DEBUG_MESSAGES>0) {
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
			getTokenName((char *) &exName,tkType);
			//getTokenName((char *) &currName, ex->type);
			//for (int i=0; i<ex->currpos; i++) putchar(' ');
			//printf("|--- match ERROR: %c, expected: %s, curr: %s\n", ex->currChar, exName, currName);
			printerror(ex,"expected",(char *)&exName);
			#pragma GCC diagnostic pop
		}
		ex->errpos = ex->currpos;
		return 0;
	}
}


/******************************************************************************/


int expr (exprparse_t * ex) {
	int result = term(ex);

	while (ex->type == PLUS || ex->type == MINUS || ex->type == AND || ex->type == OR) {
		if(ex->type == PLUS) {
			if (match(ex, PLUS)) result += term(ex);
		} else if (ex->type == MINUS) {
			if (match(ex, MINUS)) result -= term(ex);
		} else if (ex->type == AND) {
			if (match(ex, AND)) result &= power(ex);
		} else {
			if (match(ex, OR)) result |= power(ex);
		}
	}
	DBGPRINTF("%d\t EXPR \n", result);
	return result;
}

/*******************************************************************************/

#define MAXARGS 10
int factor1(exprparse_t * ex) {
	int result;
	int params[MAXARGS+1];
	int paramCount=0;

	if(ex->type == FUNCT)  {
		DBGPRINTF("%d\t EXPR FUNC S\n", result);
		getToken(ex);
		if (! match(ex, LPAREN)) return 0;
		if (ex->type != RPAREN) {
			do {
				if(paramCount>0)
					if(ex->type == COMMA) getToken(ex);
				params[paramCount]=expr(ex);
				paramCount++;
				if (paramCount == MAXARGS) {
					printf("more than arguments than supported in call to function %s\n",ex->f->functionName);
					ex->errpos = ex->currpos;
					return 0;
				}
			} while (ex->type == COMMA);
			result = ex->f->f(paramCount,params);
			if (! match(ex, RPAREN)) return 0;
		} else {
			result = ex->f->f(0,params);	// function w/o params
		}
		DBGPRINTF("%d\t EXPR FUNC E\n", result);
	} else
	if(ex->type == LPAREN){
		if (! match(ex, LPAREN)) return 0;
		result=expr(ex);
		if (! match(ex, RPAREN)) return 0;

	} else {
		getToken(ex);
		result=ex->ival;
	}
	return result;
}

/******************************************************************************/

int factor (exprparse_t * ex) {
	int result;

	if(ex->type == MINUS){
		if (! match(ex, MINUS)) return 0;
		result=(-1)*factor1(ex);
		return result;
	}

	result=factor1(ex);
	DBGPRINTF("%d\t FACTOR \n", result);
	return result;
}

/******************************************************************************/

int ipow (int base, int exp)
{
    int result = 1;
    for (;;) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }
    return result;
}

/******************************************************************************/

int power (exprparse_t * ex) {
int result = factor(ex);

	if (ex->type == POWER) {
		if (match(ex, POWER)) return ipow(result,power(ex));
		return 0;
	}
	DBGPRINTF("%d\t POWER \n", result);
	return result;
}

/******************************************************************************/

int term (exprparse_t * ex) {
	int result = power(ex);

	while (ex->type == MULT || ex->type == DIVIDE || ex->type == REMAINDER)
	{
		if(ex->type == MULT) {
			if (match(ex, MULT)) result = result * power(ex);
		} else if (ex->type == DIVIDE) {
			if (match(ex, DIVIDE)) result = result / power(ex);
		} else {
			if (match(ex, REMAINDER)) result = result % power(ex);
		}
	}
	DBGPRINTF("%d\t TERM \n", result);
	return result;
}


/******************************************************************************/



// initialize
void exprparseInit (exprparse_t * ex, char * exprString) {
	ex->currpos=0;
	ex->errpos=-1;
	ex->inputStr=exprString;
	ex->nextChar=exprString;
	ex->f = NULL;
	ex->result=0;
}

// parse
int exprparse (exprparse_t * ex) {
	getToken(ex);
	ex->result = expr(ex);
	DBGPRINTF("%d\t RESULT errpos=%d\n", ex->result,ex->errpos);
	return(ex->errpos == -1);		// return true on success
}
