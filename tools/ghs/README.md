# Green Hills C for BOSS/IX

Build C programs for BOSS/IX on the MAI 2000 with the Green Hills C-68000
1.8.3A compiler from the tapes at
http://www.basicfour.de/download/mai2000/ghs-m68k-pascal-c.tar.bz2.

The compiler on the tape is a VAX binary, so it runs on 4.3BSD in the Open
SIMH VAX-11/780. Its `-ga` output (every instruction already encoded as
`.word`) is assembled and linked by the GNU m68k binutils on the host, and
`mkrel.py` writes the `.rel` executable BOSS/IX loads. The BOSS/IX `asm`
cannot be used: it creates its temporary files with a call the BOSS/IX kernel
does not support ("Can't open temps", also on the real machine).

## Requirements

- Open SIMH `vax780` (https://github.com/open-simh/simh). The simh 3.8.1 VAX
  packaged by Ubuntu 22.04 crashes the 4.3BSD kernel ("Process PTE in P0 or
  P1 space").
- `binutils-m68k-linux-gnu`, `python3`.
- From the TUHS 4.3BSD directory: `stand.gz`, `miniroot.gz`, `rootdump.gz`,
  `usr.tar.gz`; and `boot42` from the Computer History Wiki page "Boot42".
- The Green Hills tar above, extracted.

## Steps

All scripts take their paths from the environment: `GHS_WORK` (work
directory, default `./work`), `SIMH` (the `vax780` binary), `GHS_TAPE`
(extracted `c68_p68_libr_1.8.3A_3-28-88`) and `GHS_RT`
(`GH_68K_LIBR_SRC_5-5-86/libs/68000/intops/rt.s`).

    ./install43.sh        # 4.3BSD on $GHS_WORK/bsd43/rq.dsk, a few minutes
    ./setupghs.sh         # compilers to /vax, library sources to /LIB
    ./mklibc.sh           # $GHS_WORK/lib/libc.a, rt68k.o, crt0.o
    ./ghscc.sh hello.c    # hello.rel

To get a `.rel` into BOSS/IX, `put.py` prints Business BASIC statements that
rebuild the file with `ATH()` and `WRITE RECORD`, and `emusess.py` types them
into eagleemu:

    mapfile -t C < <(python3 put.py hello.rel /tmp/hello 100)
    FAST=1 QUIET=1 PROMPT2=">" python3 emusess.py <eagleemu dir> <disk image> \
        "${C[@]}" "filemodes +rwx +rwx /tmp/hello" "/tmp/hello" "!shutdown"

About 11 KB take 8 minutes. End the session with `!shutdown`, killing the
emulator leaves the disk image unbootable.

## What the library does

`bxos.c` replaces the generic `os.c` of the tape and `crt0.s` starts the
program. Tested on BOSS/IX 7.5B: printf, stdin, argc/argv, fopen for reading
and writing, fprintf, fseek/ftell, unlink; `seektest.c` exercises most of
it. Floating point and time are not mapped.

## BOSS/IX notes

- Executables are `.rel` files: a 0x40 byte header (`.rel`, header size,
  text, data, bss, 0, symbols, relocation, 0, data base, bss base, 0, time,
  1, 0, 0), text loaded at 0 and data at 0x400000. Symbols and relocation can
  be empty.
- The kernel starts a program with 0, argc, argv and envp on the stack.
- System calls are `trap #2` with the number in A0 and the arguments in D0 to
  D3; -0x8000 means retry. Programs write to channel 2 and read from 1.

  | call | use |
  |------|-----|
  | 0d / 0e | read / write (channel, buffer, length) |
  | 66 | open (path, flags, mode); reading only, creating returns -5 |
  | 8b | native open (path, mode block or 0 for read/write) |
  | 05 | close |
  | 0a / 10 | seek to an absolute position / current position |
  | 2f | file information, 0x60 bytes, the size is the long at 14 |
  | 17 | unlink |
  | 29 | exit |

- Files are created with `trap #0` followed by the word 0x0089 and eight longs
  on the stack: type, path, 0, 0, 1, 0, 0, 0 make a string file, as Business
  BASIC `STRING` does. -37 means the file exists.
