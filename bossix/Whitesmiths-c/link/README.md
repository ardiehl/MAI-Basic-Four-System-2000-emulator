Whitesmths c compiler from https://github.com/hansake/Whitesmiths-C-compiler

# link

This is the Whitesmiths idris linker.

## Changes
- can be compiled with the original compiler on BOSS/IX and for Linux
- can create BOSS/IX executables and shared libraries using -BX or -BL, defaults to the Idris format
- can include shared libraries, e.g. -SL /sys/unos.sl
- longer help with --help

```plaintext
usage: link -[a bb## b## c db## dr# d eb* ed* et* h i l*^ +l* o* r sb* sd*  
	st* tb## tf# t u*^ x# BX BL SL*^ LV##^ V## S*^] <files>
-a    code and symbols absolute        -bb## set bss bias to ## (not eod)
-b##  pad stack + heap by ## bytes
-c    don't output code sections (.text || .data) NOTE: implies -a
-db## set data bias to ## (not end of text)
-dr#  round data bias up to 2**# byte bound (not end of text)
-d    don't define BSS symbols or complain about undefined
-eb*  def symbol as end of bss         -ed*  as end of data
-et*  def symbol as end of text        -h    suppress header
-i    get files from STDIN             -l*   library root of /lib
-o*   output to file, not 'xeq'        -r    suppress relocation bits
-sb*  define symbol as size of bss     -sd*  as size of data
-st*  define symbol as size of text    -tb## set text bias to ## (not 0)
-tf#  fill text up to 2**# byte bound  -t    suppress symbol table
-u*^  enter symbol as undefined        -x#   2=>text to text, 1=>data to text
-BX   create BOSS/IX executable        -BL  create BOSS/IX shared library
-SL   shared lib file names required   -LV  shared lib version numbers
-V    set version field in header      -S   dirs to search for shared libs```

