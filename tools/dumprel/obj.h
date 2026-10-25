#ifndef _OBJ_
#define _OBJ_

#include <inttypes.h>

#define UNDEFINED	0
#define ABSOLUTE	1
#define TEXT		2
#define DATA		3
#define BSS		4
#define REGISTER	5
#define	COMMLEN		6
#define ENDTEXTREL	012
#define ENDDATAREL	013
#define NEWBASE		014
#define SECTION		017
#define GLOBAL		020
#define LONG		040
#define PCREL		0100
#ifdef BIGSYMS
#define SYMNAMSIZE	64
#else //BIGSYMS
#define SYMNAMSIZE	16
#endif //BIGSYMS
#define RELFILE	('.rel')
#define	LOCALC		'@'

/*
    BOSS/IX extension, shared libraries
*/

typedef struct {
    uint32_t createtime;
    char libFileName[255];
} objhead_lib;

/*
	object file header
*/
struct objhead
{
	uint32_t magic,		/* file type and version number */
		headersize,	/* size of this header (bytes) */
		textsize,	/* size of text (bytes) */
		datasize,	/* size of initialized data (bytes) */
		bsssize,	/* size of uninitialized data (bytes) */
		stacksize,	/* size of stack (bytes) */
		relsize,	/* size of relocation information (bytes) */
		symsize,	/* size of symbol table (bytes) */
		textbase,	/* base address of text segment */
		database,	/* base address of data segment */
		bssbase,	/* base address of bss segment */
		/* AD: Unos only, stackbase, */	/* base address of stack segment */
		createtime,	/* time this file was made */
		unknown,
		objflags,	/* assorted flags (see below) */
		hdrrsv[2];	/* reserved */
		objhead_lib firstSharedLib;
};

#define OBJHEAD	struct objhead

/*	objflags values  */
#define LUBASE0		0x1



/*
	relocation information
*/

struct reloc
{
//#ifdef M68000
	unsigned char	r_offset;/* offset (bytes) from last relocated word  */
	unsigned char	r_type;	 /* relocation type */
//#else
//	unsigned char	r_type;	 /* relocation type */
//	unsigned char	r_offset;/* offset (bytes) from last relocated word  */
//#endif
	unsigned short	r_symno;	/* symbol number  */
};

#define REL	struct reloc

/*
	symbol table information
*/
struct __attribute__((packed)) symtabent
{
	//short	s_flags;		/* type of this entry */
	uint16_t s_flags;		/* type of this entry */
	uint32_t s_value;		/* value for defined sym or common size else 0 */
	char	s_name[SYMNAMSIZE];	/* symbol name ascii */

};
#define SYM	struct symtabent
#endif // _OBJ_
