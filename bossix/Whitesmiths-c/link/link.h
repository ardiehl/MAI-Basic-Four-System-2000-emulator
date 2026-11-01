/*	HEADER FOR IDRIS LINKER
 *	copyright (c) 1981 by Whitesmiths, Ltd.
 */

 /*	AD: if defined, the target executable can be a BOSS/IX
	executable or shared library as well */
#define BOSSIX

#ifdef BOSSIX

/* on BOSS/IX time_t starts at 01.01.1980 */
#define EPOCH1980 315532800L

/*	the list of shared libraries to be included in file header
 */
#define NBXSL	10

typedef struct {
	BYTES nm;
	TEXT *path[NBXSL];
	} SLLIST;

/* the 32 bit build time in header that has to match the header in the sl file */
typedef struct {
	BYTES nm;
	LONG version [NBXSL];
	} SLVLIST;

#endif

/* the constructed output header
 */
typedef struct {
	ULONG tsiz, dsiz, bsiz, ssiz;
	ULONG tbias, dbias;
	ULONG atbias, adbias, atsiz, adsiz;
	UCOUNT sbytes;
	} BHDR;

/*	symbol table stuff
 */
#define DEFD	004
#define EXTERN	010
#define MAXSYMS	2048
#define NTABS	32
#define SMASK	077
#define SOFF	6
#define SSIZE	12

typedef struct {
	LONG value;
	TEXT flags, name[15];
	TEXT *filnm;
	} SYMBOL;

/*	the list of undefined arguments
 */
#define NUND	10

typedef struct {
	BYTES nm;
	TEXT *sym[NUND];
	} ULIST;

/*	the list of libraries
 */
#define EXLIB	10

/* AD: increased from 128 */
#define MAXLIBS	256
typedef struct {
	BYTES num;
	TEXT *nam[EXLIB];
	} LIBLIST;

/* AD: */
#define IBUF_MAX 2
#define OBUF_MAX 4
/*	the buffer controllers
 */
#define WRMI	8
#define DRAIN	4
#define DREL	3
#define TREL	2
#define	DDAT	1
#define	TDAT	0

/*	input buffer controllers
 */
#define HIBUF	1
#define LOBUF	0

/*	buffer size parameters
 *	NOTE, must be power of 2
 *	all three must change in phase!
 */
#define L_BUFSIZE	64
#define L_BUF_MASK	077
#define	NOT_L_BUF_MASK	~077L

typedef struct {
	LONG off;
	COUNT start, n;
	UTINY *buf;
	} IOBUF;
