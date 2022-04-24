#include <inttypes.h>

#ifndef EXPRPARSE_H_INCLUDED
#define EXPRPARSE_H_INCLUDED

typedef enum {
	FUNCT,
	PLUS,
	MINUS,
	DIVIDE,
	MULT,
	REMAINDER,
	POWER,
	AND,
	OR,
	LPAREN,
	RPAREN,
	NUMBER,
	ERROR,
	COMMA,
	EOL
} TokenType;

typedef int (*funcPtr_t)(int ,int[]);

typedef struct symbol_T {
    int value;
    char * name;
    char * desc;
    struct symbol_T * next;
    uint8_t nameAlloced;
} symbol_t;

typedef struct func_T {
	char * functionName;
	char * desc;
	//int (*f)(int argc, int argv[]);
	funcPtr_t f;
	struct func_T * next;
} func_t;


typedef struct {
	char * inputStr;	// null terminated string to parse
	int    currpos;		// current position in string
	char * nextChar;	// will be returned on next ex_getchar
	int    errpos;		// >-1 for error pos
	int    result;		// final result value
	TokenType type;		// current token type
	int    ival;
	int    currChar;	// last returned char
	func_t * f;		// current function
} exprparse_t;



int exprparseAddSymbol (char * name, int value, char * desc, int allocNameMemory);
int exprparseDeleteSymbol (char * name);	// -1=not found else 0
int exprparseFindSymbol (char * name, symbol_t ** sym);  // -1=not found else value
void expparseListSymbols ();

//void exprparseAddFunction (char *name, int *(int argc, int argv[]) );
void exprparseAddFunction (char *name, funcPtr_t);

// initialize exprparse_T struct
void exprparseInit (exprparse_t * ex, char * exprString);

// parse, returns 0 for ok or errorcode, result is in ex->result
int exprparse (exprparse_t * ex);
void expparseListFunctions ();

#endif // EXPRPARSE_H_INCLUDED
