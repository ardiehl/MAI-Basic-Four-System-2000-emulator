#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <endian.h>
#include "obj.h"
#include "m68k.h"
#include "argparse.h"

FILE * fin;

void usage (char * pname) {
	printf(
		"usage:\n" \
		"%s: unosobjfilename newstacksize\n",pname);
	exit (1);
}


/* returns hex digit for lower 4 bits */
char hexNibble (unsigned int i)
{
        char digits[16] = "0123456789abcdef";
        return digits[i & 0x0f];
}


void showDump (uint8_t * data,unsigned int addr, unsigned int length, int lineLen) {
        uint8_t d;
        int asciiLen = 0;
        int hexLen;
        char hexData[255];
        char ascii[lineLen+1];
        int count = 0;

        //printf("\n");
        hexData[0]=0; sprintf(hexData,"%08x: ",addr);
        hexLen = strlen(hexData);
        while (count < length) {
				count++;
                d = *data; data++;
                if ((d < ' ') | (d > 0x7e)) ascii[asciiLen++] = '.'; else ascii[asciiLen++] = d;
                ascii[asciiLen] = 0;
                hexData[hexLen++] = hexNibble(d >> 4);
                hexData[hexLen++] = hexNibble(d);
                hexData[hexLen++] = ' ';
                hexData[hexLen] = 0;
                if (asciiLen == 8) { hexData[hexLen++] = ' '; hexData[hexLen] = 0; }
                addr++;
                if (asciiLen == lineLen) {
                        printf("%-59s  %s\n",hexData,ascii);
                        asciiLen = 0;
                        hexData[0]=0; sprintf(hexData,"%08x: ",addr);
                        hexLen = strlen(hexData);
                }
        }
        if (asciiLen > 0) {
                printf("%-59s  %s\n",hexData,ascii);
        }
}


int showText,showData,showReloc,showSym,showSyms;
int newStackSize = -1;

int allCB(argParse_handleT *a, char * arg) {
  showText++; showData++; showReloc++; showSym++; showSyms++;
  return 0;
}

AP_START(argopt)
        AP_HELP
        AP_OPT_INTVALF_CB (0,'a',"all"  ,NULL       ,"show all sections",allCB)
        AP_OPT_INTVALF    (0,'t',"text"  ,&showText ,"show text section")
        AP_OPT_INTVALF    (0,'d',"data"  ,&showData ,"show data section")
        AP_OPT_INTVALF    (0,'r',"rel"  ,&showReloc ,"show relocation section")
        AP_OPT_INTVALF    (0,'S',"sym"  ,&showSym   ,"show symbols section")
        AP_OPT_INTVALF    (0,'s',"syms" ,&showSyms  ,"show symbols")
        //AP_OPT_STRVAL     (1,'s',"str"   ,&strArg    ,"set string")
        AP_OPT_INTVAL     (0,'z', "stacksize"  ,&newStackSize   ,"set new stacksize")
        //AP_OPT_INTVALFO   (0,'I',"int2"  ,&intArg2   , "increment or set int")
        //AP_OPT_INTVAL_CB  (1,'c', "intc" ,&intArgC   ,"set int with callback", &callback)
        //AP_OPT_INTVAL_CB  (1,'c', NULL ,&intArgC   ,"set int with callback", &callback)
        //AP_OPT_INTVAL     (0,0, "int"  ,&intArg1   ,"set int")
        //AP_OPT_INTVALFO   (0,0,"int2"  ,&intArg2   , "increment or set int")
AP_END;





uint8_t *text;
uint32_t textStart = 0;
uint32_t textSize;

uint8_t * textAddressPtr (uint32_t address) {
	if (address < textStart) return NULL;
	if (address > textStart+textSize) return NULL;
	return text + address - textStart;
}

uint32_t m68k_read_disassembler_32 (uint32_t address) {
	uint8_t * a = textAddressPtr (address);
	if (!address) return 0xffffffff;
	return ((uint32_t)*a << 24) | ((uint32_t)(*(a+1)) << 16) | ((uint32_t)(*(a+2)) << 8) | *(a+3);
}

unsigned int m68k_read_disassembler_16 (uint32_t address) {
	uint8_t * a = textAddressPtr (address);
	if (!address) return 0xffffffff;

	return (((uint32_t)*a) << 8) | *(a+1);
}

unsigned int m68k_read_disassembler_8 (uint32_t address) {
	uint8_t * a = textAddressPtr (address);
	if (!address) return 0xffffffff;
	return *a;
}


int main (int argc, char * argv[]) {
	struct objhead h;
	uint32_t dataStart = 0;
	int r;
	int rc;
	argParse_handleT *a;
	char *fileName = NULL;

	a = argParse_init(argopt, NULL, "\ndumprel - show information for a BOSS/IX or Unos executable or shared library\n","Filename has to be the last parameter\n");
	rc = argParse (a, argc, argv, 1);
	if (rc >= 0)
		if (a->optArgsCount) fileName = strdup(argParse_getOptArg(a,0));

	if (rc < 0) return 1;

	if (!fileName) {
		argParse_showHelp(a, NULL);
	}
	argParse_free (a);

	fin = fopen(fileName,"rb");
	if (!fin) {
		printf("%s: unable to open \"%s\"\n",argv[0],fileName);
		exit(2);
	}
	r = fread(&h,1,sizeof(h.magic)+sizeof(h.headersize),fin);
	if (r != sizeof(h.magic)+sizeof(h.headersize)) {
		printf("error reading header, expected to get %ld bytes but got %d bytes\n",sizeof(h.magic)+sizeof(h.headersize),r);
		exit(3);
	}

	if ( (strcmp (  (char *) &h.magic, ".rel") != 0) &&  (strcmp (  (char *) &h.magic, ".lib") != 0))  {
		printf("unknown file header\n");
		exit(4);
	}
	int hdrSize = be32toh(h.headersize);
	printf("size of this header (bytes)           : 0x%08x (%d)\n", hdrSize, hdrSize);

	r = fread(&h.textsize,1,hdrSize-sizeof(h.magic)-sizeof(h.headersize),fin);
	if (r != (hdrSize-sizeof(h.magic)-sizeof(h.headersize))) {
		printf("error reading remaining header, expected to get %ld bytes but got %d bytes\n",hdrSize-sizeof(h.magic)-sizeof(h.headersize),r);
		exit(3);
	}

	if (newStackSize > -1) {
		fclose(fin);
		int newStackSize = strtol(argv[2],NULL,10);
		h.stacksize = htobe32(newStackSize);
		fin = fopen(argv[1],"r+b");
		if (fin) {
			fwrite(&h,1,hdrSize,fin);
			fclose(fin);
			return 0;
		} else {
			printf("unable to write\n");
			return 1;
		}
	}
	//printf("long    magic,          /* file type and version number */


	textSize = be32toh(h.textsize);
	printf("size of text (bytes)                  : 0x%08x (%d)\n", textSize,textSize);
	printf("size of initialized data (bytes)      : 0x%08x (%d)\n", be32toh(h.datasize),be32toh(h.datasize));
	printf("size of uninitialized data (bytes)    : 0x%08x (%d)\n", be32toh(h.bsssize),be32toh(h.bsssize));
	printf("size of stack (bytes)                 : 0x%08x (%d)\n", be32toh(h.stacksize),be32toh(h.stacksize));
	printf("size of relocation information (bytes): 0x%08x (%d)\n", be32toh(h.relsize),be32toh(h.relsize));
	printf("size of symbol table (bytes)          : 0x%08x (%d)\n", be32toh(h.symsize),be32toh(h.symsize));
	if (hdrSize > 32) {
		textStart = be32toh(h.textbase);
		printf("base address of text segment          : 0x%08x\n", textStart);
		dataStart = be32toh(h.database);
		printf("base address of data segment          : 0x%08x\n", dataStart);
		printf("base address of bss segment           : 0x%08x\n", be32toh(h.bssbase));
		printf("base address of stack segment         : 0x%08x\n", be32toh(h.stackbase));
		//printf("        createtime,     /* time this file was made */
		printf("assorted flags (see below)            : 0x%08x\n", be32toh(h.objflags));
		printf("reserved 0x%02x 0x%02x\n",h.hdrrsv[0],h.hdrrsv[1]);
	}


	if (be32toh(h.textsize) > 0) {
		text = (uint8_t *)calloc(1,be32toh(h.textsize));
		if (!text) {
			printf("failed to allocate %d bytes\n",be32toh(h.textsize));
			return 1;
		}
		r = fread(text,1,be32toh(h.textsize),fin);
		if (r != be32toh(h.textsize)) {
			printf("error reading text segment, expected to get %d bytes but got %d bytes\n",be32toh(h.textsize),r);
			exit(3);
		}
		if (showText) {
			printf("\n------ text -----------------------------------------------------------------\n");
			showDump (text,textStart,be32toh(h.textsize), 16);
		}
	}

	uint8_t *data;
	if (be32toh(h.datasize) > 0) {
		data = (uint8_t *)calloc(1,be32toh(h.datasize));
		if (!data) {
			printf("failed to allocate %d bytes\n",be32toh(h.datasize));
			return 1;
		}
		r = fread(data,1,be32toh(h.datasize),fin);
		if (r != be32toh(h.datasize)) {
			printf("error reading data segment, expected to get %d bytes but got %d bytes\n",be32toh(h.datasize),r);
			exit(3);
		}
		if (showData) {
			printf("\n------ data -----------------------------------------------------------------\n");
			showDump (data,dataStart,be32toh(h.datasize), 16);
		}
	}

	uint8_t *reloc;
	if (be32toh(h.relsize) > 0) {
		reloc = (uint8_t *)calloc(1,be32toh(h.relsize));
		if (!data) {
			printf("failed to allocate %d bytes\n",be32toh(h.relsize));
			return 1;
		}
		r = fread(reloc,1,be32toh(h.relsize),fin);
		if (r != be32toh(h.relsize)) {
			printf("error reading relocation data, expected to get %d bytes but got %d bytes\n",be32toh(h.relsize),r);
			exit(3);
		}
		if (showReloc) {
			printf("\n------ relocation data ------------------------------------------------------\n");
			showDump (reloc,0,be32toh(h.relsize), 16);
		}
	}

	uint8_t *syms;
	if (be32toh(h.symsize) > 0) {
		syms = (uint8_t *)calloc(1,be32toh(h.symsize));
		if (!syms) {
			printf("failed to allocate %d bytes\n",be32toh(h.symsize));
			return 1;
		}
		r = fread(syms,1,be32toh(h.symsize),fin);
		if (r != be32toh(h.symsize)) {
			printf("error reading symbols, expected to get %d bytes but got %d bytes\n",be32toh(h.symsize),r);
			exit(3);
		}
		if (showSym || showSyms) {
			printf("\n------ symbols --------------------------------------------------------------\n");
			showDump (syms,0,be32toh(h.relsize), 16);
		}

		if (showSyms) {
			struct symtabent *sym = (struct symtabent *)(syms);
			while (sym < (struct symtabent *) (syms + be32toh(h.symsize))) {
				int flags = be16toh(sym->s_flags);
				printf("%04x %08x %-20s",flags,be32toh(sym->s_value),sym->s_name);
				if (flags == 0x12) { // || flags == 2) {
					if (textAddressPtr(be32toh(sym->s_value))) {
						char disTxt[255];
						m68k_disassemble(disTxt, be32toh(sym->s_value), M68K_CPU_TYPE_68010);
						printf("%s",disTxt);
					}
				}
				printf("\n");
				sym++;
			}
		}
	}

}
