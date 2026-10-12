#!/bin/bash
# Compile and link C files into a BOSS/IX .rel executable named after the
# first file.   ./ghscc.sh prog.c [more.c ...]
set -e
D=$(cd "$(dirname "$0")" && pwd)
L=${GHS_WORK:-$D/work}/lib
"$D/vaxcc.sh" "$@"
objs=
for f in "$@"; do
	m68k-linux-gnu-as -m68000 -o "${f%.c}.o" "${f%.c}.s"
	objs="$objs ${f%.c}.o"
done
out=${1%.c}
m68k-linux-gnu-ld -N -e _start -Ttext=0 -Tdata=0x400000 -o $out.elf $L/crt0.o $objs $L/libc.a $L/rt68k.o
python3 "$D/mkrel.py" $out.elf $out.rel
