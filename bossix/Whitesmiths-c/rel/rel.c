/*	INSPECT IDRIS OBJECT FILES
 * 	copyright (c) 1981 by Whitesmiths, Ltd.
 */
#include <std.h>

/*	symbol types
 */
#define UDEFSYMB	0
#define ABSSYMB		4
#define TXTSYMB		5
#define DATASYMB	6
#define BSSSYMB		7
#define EXT			010

/*	header sizes
 */
#define MAGSIZ		2
#define LNGLIB		26
#define SRTLIB		16
#define LNGHDR		26
#define SRTHDR		14

/*	flags:
	-d		list defined symbols
	-g		list global symbols
	-h		output in hex (used with -s flag)
	-i		print intervals in value field
	-o		output in octal (default is hex)
	-s		tell sizes of text, data, bss and total
	-t		tell machine and library type data
	-u		list undefined symbols
	-v		sort by value
 */
BOOL dflag = {NO};
BOOL gflag = {NO};
BOOL hflag = {NO};
BOOL iflag = {NO};
BOOL manyfls = {NO};
BOOL oflag = {NO};
BOOL sflag = {NO};
BOOL tflag = {NO};
BOOL uflag = {NO};
BOOL vflag = {NO};

TEXT *_pname = {"rel"};

/*	decoded header values
 */
BOOL evenbnd = {NO};
BOOL longint = {NO};
BOOL lsfmt  = {NO};
BOOL txtrev = {NO};
COUNT lenname = {0};
UCOUNT binhdr = {0};
UCOUNT sbytes = {0};
ULONG bsize = {0};
ULONG dbias = {0};
ULONG dsize = {0};
ULONG ssize = {0};
ULONG tbias = {0};
ULONG tsize = {0};

/*	running totals
 */
ULONG glbsize = {0};
ULONG gldsize = {0};
ULONG glssize = {0};
ULONG gltsize = {0};

/*	symbol table stats
 */
TEXT *norder= {NULL};		/* pointer to unordered symbol table */
TEXT **sorted = {NULL};	/* pointer to array of pointers to sorted entries */
COUNT numsym = {0};
COUNT entsize = {0};
COUNT valsize = {0};

/*	find actual length of a name
 */
BYTES actlen(s)
	FAST TEXT *s;
	{
	IMPORT COUNT lenname;
	FAST BYTES i;

	for (i = 0; i < lenname &&  *s++ ; ++ i)
		;
	return (i);
	}

/*	do a file, object or library
 */
BOOL dofile(fd, fname)
#ifdef USESTDCLIB
	int fd;
#else
	FAST FILE fd;
#endif
	TEXT *fname;
	{
	IMPORT BOOL manyfls, sflag;
#ifdef USESTDCLIB
	IMPORT UCOUNT binhdr, getmagic();
#else
	IMPORT UCOUNT binhdr, getmagic(), lstoi();
#endif
	FAST COUNT hsize;
	LONG loff;
	TEXT lhdr[LNGLIB];
	UCOUNT header;

	header = getmagic(fd);
	if (header ==  binhdr)
		{
		if (manyfls)
			{
			putstr(STDOUT, fname, NULL);
			if (sflag)
				putstr(STDOUT, ": ", NULL);
			else
				putstr(STDOUT, ":\n", NULL);
			}
		intrpfil(fd);
		}
	else if (header !=  0177565 &&  header !=  0177555 &&  header !=  0177545)
		return (remark("bad file format: ", fname));
	else
		{
		if (tflag)
			{
			putstr(STDOUT, "Library - ", fname, " - ", NULL);
			switch (header)
				{
			case 0177565:
				putstr(STDOUT, "standard format.\n", NULL);
				break;
			case 0177555:
				putstr(STDOUT, "Unix V6 format.\n", NULL);
				break;
			case 0177545:
				putstr(STDOUT, "Unix V7 format.\n", NULL);
				}
			}
		manyfls = YES;
		hsize = (header ==  0177545) ? LNGLIB : SRTLIB;
		for (loff = 2; (fread(fd, &lhdr, hsize) ==  hsize) &&  lhdr[0];
			lseek(fd, loff, 0))
			{
			if (getmagic(fd) !=  binhdr)
				return (remark("bad file in library: ", fname));
			else
				{
				putstr(STDOUT, &lhdr, (sflag) ? ": " : ":\n", NULL);
				intrpfil(fd);
				}
			loff +=  hsize + ((header !=  0177545) ? (ULONG)lstoi(&lhdr[14])
				: ((ULONG)lstoi(&lhdr[22]) <<  16) + lstoi(&lhdr[24]));
			if (header !=  0177565)
				loff +=  loff & 1;
			}
		}
	return (YES);
	}

/*	give the ordering of two pointer list entries by name
 */
COUNT entorn(i, j, plist)
	COUNT i, j;
	TEXT **plist;
	{
	IMPORT BOOL longint;
	IMPORT COUNT lenname;
	IMPORT TEXT **sorted;
	FAST COUNT u;
	FAST TEXT *s;
	FAST TEXT *t;

	s = sorted[i] + ((longint) ? 5 : 3);
	t = sorted[j] + ((longint) ? 5 : 3);
	for (u = lenname; 0 < u; -- u, ++ s, ++ t)
		if (*s !=  *t)
			return (*s - *t);
	return (0);
	}

/*	give the ordering of two pointer list entries
 *	by value else call entorn
 */
COUNT entorv(i, j, plist)
	COUNT i, j;
	TEXT **plist;
	{
	IMPORT BOOL longint;
	IMPORT TEXT **sorted;
	FAST ULONG s;
	FAST ULONG t;

	s = (longint) ? *(ULONG *)(sorted[i]) : *(UCOUNT *)(sorted[i]);
	t = (longint) ? *(ULONG *)(sorted[j]) : *(UCOUNT *)(sorted[j]);
	if (s ==  t)
		return (entorn(i, j, plist));
	if (s < t)
		return (-1);
	else
		return (1);
	}

/*	exchange two entries
 */
VOID entswap(i, j, plist)
	COUNT i, j;
	TEXT **plist;
	{
	IMPORT TEXT **sorted;
	FAST TEXT *tmp;

	tmp = sorted[i];
	sorted[i] = sorted[j];
	sorted[j] = tmp;
	}

/*	get header information from file
 */
VOID gethdr(fd)
#ifdef USESTDCLIB
	int fd;
#else
	FILE fd;
#endif
	{
	IMPORT BOOL longint;
	IMPORT UCOUNT sbytes, xstos();
	IMPORT ULONG xstol();
	IMPORT ULONG bsize, dbias, dsize, ssize, tbias, tsize;
	COUNT hsize = (longint) ? LNGHDR : SRTHDR;
	TEXT bhdr[LNGHDR];

	if (fread(fd, bhdr, hsize) !=  hsize)
		error("can't read binary header", NULL);
	sbytes = xstos(&bhdr[0]);
	if (longint)
		{
		tsize = xstol(&bhdr[2]);
		dsize = xstol(&bhdr[6]);
		bsize = xstol(&bhdr[10]);
		ssize = xstol(&bhdr[14]);
		tbias = xstol(&bhdr[18]);
		dbias = xstol(&bhdr[22]);
		}
	else
		{
		tsize = xstos(&bhdr[2]);
		dsize = xstos(&bhdr[4]);
		bsize = xstos(&bhdr[6]);
		ssize = xstos(&bhdr[8]);
		tbias = xstos(&bhdr[10]);
		dbias = xstos(&bhdr[12]);
		}
	}

/*	get magic number from header
 */
UCOUNT getmagic(fd)
#ifdef USESTDCLIB
	int fd;
#else
	FILE fd;
#endif
	{
	IMPORT BOOL longint, lsfmt, evenbnd, txtrev;
	IMPORT COUNT lenname;
	IMPORT UCOUNT binhdr;
	FAST UCOUNT hdr;
	UTINY header[MAGSIZ];

	if (fread(fd, &header, MAGSIZ) !=  MAGSIZ)
		error("can't read header", NULL);
	hdr = lstoi(header);
	if (header[0] ==  0x99)
		{
		lenname = ((header[1] & 07) <<  1) + 1;
		longint = header[1] & 010;
		lsfmt = header[1] & 020;
		evenbnd = header[1] & 040;
		txtrev = header[1] & 0100;
		binhdr = hdr;
		}
	return (hdr);
	}

/*	get symbol table and set table stats
*/
TEXT *getsym(fd)
#ifdef USESTDCLIB
	int fd;
#else
	FILE fd;
#endif
	{
	IMPORT BOOL longint;
	IMPORT COUNT lenname, numsym, entsize, valsize;
	IMPORT TEXT **sorted;
	IMPORT UCOUNT sbytes, xstos();
	IMPORT ULONG dsize, tsize, xstol();
	FAST COUNT i;
	FAST TEXT *ps, *tps;

	valsize = (longint) ? 4 : 2;
	entsize = valsize + 1 + lenname;
	numsym = sbytes / entsize;
	lseek(fd, tsize + dsize, 1);
	ps = alloc(sbytes, NULL);
	sorted = alloc(sizeof (TEXT *) * numsym, NULL);
	if (fread(fd, ps, sbytes) !=  sbytes)
		error("can't read symbol table", NULL);
	for (i = 0, tps = ps; i < numsym ; ++ i, tps +=  entsize)
		{
		if (longint)
			*(ULONG *)tps = xstol(tps);
		else
			*(UCOUNT *)tps = xstos(tps);
		sorted[i] = tps;
		}
	return (ps);
	}

/*	get next extern from entry table
 */
COUNT getext(list, start, total)
	COUNT start, total;
	TEXT **list;
	{
	IMPORT BOOL longint;
	FAST COUNT i;
	FAST COUNT findx = (longint) ? 4 : 2;
	FAST TEXT *p;

	for (i = start; i < total; ++ i)
		{
		p = list[i];
		if (p[findx] & EXT)
			return (i);
		}
	return (-1);
	}

/*	play with header while reacting to flags
 */
VOID intrpfil(fd)
#ifdef USESTDCLIB
	int fd;
#else
	FILE fd;
#endif
	{
	IMPORT BOOL evenbnd, gflag, sflag, tflag, uflag;
	IMPORT BOOL hflag, dflag, longint, lsfmt, txtrev;
	IMPORT COUNT lenname;
	IMPORT ULONG bsize, dsize, ssize, tsize;
	IMPORT ULONG glbsize, gldsize, glssize, gltsize;

	gethdr(fd);
	if (tflag)
		{
#ifdef USESTDCLIB
		printf("%d byte integers, data %s significant byte first.\n",
			(longint) ? 4 : 2, (lsfmt) ? "least" : "most");
		if (evenbnd)
			printf("even byte boundaries enforced by hardware.\n");
		if (txtrev)
			printf("text segment int byte order is reversed from data segment.\n");
		printf("%d character names max.\n", lenname);
#else
		putfmt("%s byte integers, data %p significant byte first.\n",
			(longint) ? 4 : 2, (lsfmt) ? "least" : "most");
		if (evenbnd)
			putfmt("even byte boundaries enforced by hardware.\n");
		if (txtrev)
			putfmt("text segment int byte order is reversed from data segment.\n");
		putfmt("%s character names max.\n", lenname);
#endif
		}
	if (sflag)
		{
#ifdef USESTDCLIB
		if (hflag)
			printf("0x%xT + 0x%xD + 0x%xB + 0x%xS = 0x%x\n",
				tsize, dsize, bsize, ssize, tsize + dsize + bsize + ssize);
		else
			printf("%dT + %oD + %oB + %oS = %o\n",
				tsize, dsize, bsize, ssize, tsize + dsize + bsize + ssize);
#else
		if (hflag)
			putfmt("0x%hlT + 0x%hlD + 0x%hlB + 0x%hlS = 0x%hl\n",
				tsize, dsize, bsize, ssize, tsize + dsize + bsize + ssize);
		else
			putfmt("%ulT + %ulD + %ulB + %ulS = %ul\n",
				tsize, dsize, bsize, ssize, tsize + dsize + bsize + ssize);
#endif
		gltsize +=  tsize;
		gldsize +=  dsize;
		glbsize +=  bsize;
		glssize +=  ssize;
		}
	if (dflag ||  uflag ||  gflag)
		lstsmbls(fd);
	}

/*	list symbols from the symbol table
 */
VOID lstsmbls(fd)
#ifdef USESTDCLIB
	int fd;
#else
	FILE fd;
#endif
	{
	IMPORT BOOL dflag, iflag, uflag, vflag;
	IMPORT COUNT entorn();
	IMPORT COUNT entorv();
	IMPORT COUNT numsym;
	IMPORT TEXT *norder, **sorted;
	IMPORT VOID entswap();
	FAST UCOUNT i;

	if ((norder = getsym(fd)) ==  NULL)
		return;
	sort(numsym, (vflag ? &entorv : &entorn), &entswap, sorted);
	if (iflag)
		warplst(sorted, numsym, tsize + dsize + bsize);
	for (i = 0; i < numsym; ++ i)
		putsymb(sorted[i]);
#ifdef USESTDCLIB
	free(norder);
	free(sorted);
#else
	free(norder, NULL);
	free(sorted, NULL);
#endif
	}

/*	disassemble and tell unresolved references
 */
BOOL main(ac, av)
	BYTES ac;
	TEXT **av;
	{
	IMPORT BOOL manyfls;
	IMPORT ULONG gltsize, gldsize, glbsize, glssize;
	FAST BOOL success;
#ifdef USESTDCLIB
	int fd;
#else
	FAST FILE fd;
#endif
	INTERN TEXT *dfile = {"xeq"};

	getflags(&ac, &av, "d,g,h,i,o,s,t,u,v:F <files>", &dflag, &gflag,
		&hflag, &iflag, &oflag, &sflag, &tflag, &uflag, &vflag);
	if (vflag &&  !gflag)
		dflag = YES;
	if (iflag)
		dflag = uflag = vflag = YES;
	if (!(sflag ||  uflag ||  dflag ||  gflag ||  tflag))
		dflag = uflag = YES;
	manyfls = (1 < ac) ? YES : NO;
	success = YES;
	if (!ac)
		{
		ac = 1;
		av = &dfile;
		}
	while (0 <=  (fd = getbfiles(&ac, &av, STDIN, STDERR, YES)))
		if (fd ==  STDERR)
			success = remark("can't read ", av[-1]);
		else
			{
			success &=  dofile(fd, av[-1]);
			close(fd);
			}
	if (sflag &&  manyfls)
		{
#ifdef USESTDCLIB
		if (hflag)
			printf("TOTAL: 0x%xT + 0x%xD + 0x%xB + 0x%xS = 0x%x\n",
				gltsize, gldsize, glbsize, glssize,
				gltsize + gldsize + glbsize + glssize);
		else
			printf("TOTAL: %oT + %oD + %oB + %dS = %o\n",
				gltsize, gldsize, glbsize, glssize,
				gltsize + gldsize + glbsize + glssize);
#else
		if (hflag)
			putfmt("TOTAL: 0x%hlT + 0x%hlD + 0x%hlB + 0x%hlS = 0x%hl\n",
				gltsize, gldsize, glbsize, glssize,
				gltsize + gldsize + glbsize + glssize);
		else
			putfmt("TOTAL: %ulT + %ulD + %ulB + %ulS = %ul\n",
				gltsize, gldsize, glbsize, glssize,
				gltsize + gldsize + glbsize + glssize);
#endif
		}
	return (success);
	}

/*	output a symbol entry
 */
VOID putsymb(pent)
	FAST TEXT *pent;
	{
	IMPORT BOOL dflag, gflag, longint, oflag, uflag;
	FAST ULONG value;
	FAST UTINY flags = pent[(longint) ? 4 : 2];
	BOOL isdef, isext;
	BYTES len;
#ifdef USESTDCLIB
	TEXT *lfmt = (oflag) ? "0%+011o%c %.*s\n" : "0x%+08x%c %.*s\n";
#else
	TEXT *lfmt = (oflag) ? "0%+011ol%ac %b\n" : "0x%+08hl%ac %b\n";
#endif
	TEXT outtype;
#ifdef USESTDCLIB
	TEXT *sfmt = (oflag) ? "0%+06o %a %.*s\n" : "0x%+04x %c %.*s\n";
#else
	TEXT *sfmt = (oflag) ? "0%+06os%ac %b\n" : "0x%+04hs%ac %b\n";
#endif
	UCOUNT wo;

	if (longint)
		{
		value = *(ULONG *)pent;
		pent +=  5;
		}
	else
		{
		value = *(UCOUNT *)pent;
		pent +=  3;
		}
	isdef = (flags & 07) !=  UDEFSYMB;
	isext = flags & EXT;
	len = actlen(pent);
	if (iflag &&  !isext)
		return;
	if (uflag &&  !dflag &&  !isdef &&  !gflag)
#ifdef USESTDCLIB
		printf("%.*s\n",len, pent);
#else
		putfmt("%b\n", pent, len);
#endif
	else if (dflag ||  gflag)
		{
		switch (flags & 07)
			{
		case ABSSYMB:
			outtype = 'a';
			break;
		case TXTSYMB:
			outtype = 't';
			break;
		case DATASYMB:
			outtype = 'd';
			break;
		case BSSSYMB:
			outtype = 'b';
			break;
		case UDEFSYMB:
			outtype = 'u';
			break;
		default:
			outtype = '?';
			break;
			}
		outtype = isext ? toupper(outtype) : outtype;
		if ((gflag &&  isext) ||  (dflag &&  isdef) ||  (uflag &&  !isdef))
			if (longint)
#ifdef USESTDCLIB
				printf(lfmt, value, outtype, len, pent);
			else
				printf(sfmt, wo = value, outtype, len, pent);
#else
				putfmt(lfmt, value, outtype, pent, len);
			else
				putfmt(sfmt, wo = value, outtype, pent, len);
#endif
		}
	}

/*	force symbols to contain intervals in value field
 */
VOID warplst(list, numsyms, totwords)
	TEXT **list;
	UCOUNT numsyms;
	ULONG totwords;
	{
	IMPORT BOOL longint;
	FAST TEXT *p, *q;
	COUNT top, next;

	if ((top = getext(list, 0, numsym)) < 0)
		return;
	p = list[top];
	while (0 < (next = getext(list, top + 1, numsym)))
		{
		q = list[next];
		if (longint)
			*(ULONG *)p = *(ULONG *)q - *(ULONG *)p;
		else
			*(UCOUNT *)p = *(UCOUNT *)q - *(UCOUNT *)p;
		top = next;
		p = q;
		}
	if (longint)
		*(ULONG *)p = totwords - *(ULONG *)p;
	else
		*(UCOUNT *)p = totwords - *(UCOUNT *)p;
	}

/*	get a long into proper byte order
 */
ULONG xstol(s)
	FAST UTINY *s;
	{
	IMPORT BOOL lsfmt;
	FAST COUNT i;
	FAST ULONG val;

	val = 0;
	if (lsfmt)
		for (i = 4; 0 <=  -- i; )
			val = val <<  8 | s[i];
	else
		for (i = 0; i < 4; ++ i)
			val = val <<  8 | s[i];
	return (val);
	}


/*	get a short in proper byte order
 */
UCOUNT xstos(s)
	FAST UTINY *s;
	{
	IMPORT BOOL lsfmt;
	FAST UCOUNT val;

	if (lsfmt)
		val = s[0] | s[1] <<  8;
	else
		val = s[0] <<  8 | s[1];
	return (val);
	}
