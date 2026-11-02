/*	STRUCTURES FOR PASS 0
 *	copyright (c) 1978 by Whitesmiths, Ltd.
 */

#ifdef USESTDCLIB
void ws_perror(TEXT *s, ...);
#endif


#ifdef LONGNAME
#define LENNAME	32
#else
#define LENNAME	8
#endif

/*	the types
 */
#define PCHCON	1
#define PEOL	2
#define PIDENT	3
#define PNUM	4
#define PPUNCT	5
#define PSTRING	6

/*	the keywords
 */
#define PDEFINE	10
#define PELSE	11
#define PENDIF	12
#define PIF		13
#define PIFDEF	14
#define PIFNDEF	15
#define PINCLUD	16
#define PLINE	17
#define PSHARP	18
#define PUNDEF	19

/*	the token type
 */
#define LEX		int

/*	the linked list of included files
 */
#define INCL	struct incl

struct incl
	{
	TEXT *next;
	TEXT *fname;
	COUNT nline;
#ifdef USESTDCLIB
	FILE *pfio;
#else
	FIO pfio;
#endif
	};

/*	the predefined keywords
 */
#define PRETAB	struct pretab

struct pretab
	{
	TEXT *prename;
	LEX pretype;
	};

/*	the linked list of tokens
 */
#ifdef USESTDCLIB
typedef struct TLIST TLIST;

struct TLIST
	{
	TLIST *next;
	LEX type;
	TEXT *white;
	BYTES nwhite;
	TEXT *text;
	BYTES ntext;
	};

#else
#define TLIST	struct tlist

struct tlist
	{
	TEXT *next;
	LEX type;
	TEXT *white;
	BYTES nwhite;
	TEXT *text;
	BYTES ntext;
	};
#endif

/*	the argument list structure
 */
#define ALIST	struct alist

struct alist
	{
	TEXT *next;
	TLIST *astart;
	TLIST *aend;
	};

/*	the symbol table definition entry
 */
#define DEF		struct def
#define NHASH	128
#define STRSIZE	512

struct def
	{
	TEXT *next;
	BYTES dnlen;
	TEXT *defn;
	TEXT dname[LENNAME];
	};

/*	the argument stack
 */
#define ARGS	struct args
#define NDARGS	10

struct args
	{
	BYTES ntop;
	TEXT *anames[NDARGS];
	};
