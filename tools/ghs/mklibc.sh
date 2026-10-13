#!/bin/bash
# Build the Green Hills C library for BOSS/IX: the tape libc sources (minus
# its generic os.c, crt0.c and test-c.c) plus bxos.c compiled on the VAX,
# assembled with the GNU m68k assembler into $GHS_WORK/lib/libc.a, and the
# 32 bit arithmetic runtime rt.s from the GH_68K_LIBR_SRC tape.
#   GHS_TAPE=.../c68_p68_libr_1.8.3A_3-28-88 ./mklibc.sh
# GHS_RT defaults to the GH_68K_LIBR_SRC_5-5-86 directory next to GHS_TAPE.
set -e
D=$(cd "$(dirname "$0")" && pwd)
GHS_RT=${GHS_RT:-$GHS_TAPE/../GH_68K_LIBR_SRC_5-5-86/libs/68000/intops/rt.s}
[ -d "$GHS_TAPE/LIB/libc" ] || { echo "GHS_TAPE: no LIB/libc in '$GHS_TAPE'"; exit 1; }
[ -f "$GHS_RT" ] || { echo "GHS_RT: no rt.s at '$GHS_RT'"; exit 1; }
L=${GHS_WORK:-$D/work}/lib
AS="m68k-linux-gnu-as -m68000"
mkdir -p $L/src
for f in "$GHS_TAPE"/LIB/libc/*.c; do
	case $(basename $f) in os.c|crt0.c|test-c.c) ;; *) cp $f $L/src/ ;; esac
done
cp "$D/bxos.c" $L/src/
"$D/vaxcc.sh" $L/src/*.c
for s in $L/src/*.s; do $AS -o ${s%.s}.o $s; done
m68k-linux-gnu-ar rcs $L/libc.a $L/src/*.o
$AS --mri -o $L/rt68k.o "$GHS_RT"
$AS -o $L/crt0.o "$D/crt0.s"
echo "built $L/libc.a, rt68k.o and crt0.o"
