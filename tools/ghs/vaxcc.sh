#!/bin/bash
# Compile C files with the Green Hills ccom68 on the 4.3BSD VAX, with the
# options the Unos cc uses, and leave the .s files next to the sources.
#   ./vaxcc.sh file.c ...
set -e
D=$(cd "$(dirname "$0")" && pwd)
B=${GHS_WORK:-$D/work}/bsd43
export SIMH=${SIMH:-vax780}
W=$(mktemp -d)
for f in "$@"; do cp "$f" $W/; done
names=$(cd $W && ls *.c)
OPTS="-ga -X12 -X14 -X16 -X22 -X81 -X99 -X102 -X122 -X138 -Dunix -Dunosccom68 -DM68000 -D__GREENHILLS -I/LIB/include"
printf 'for f in *.c; do /vax/compiler/ccom68 %s $f; done\n' "$OPTS" > $W/cc.sh
(cd $W && tar cf in.tar $names cc.sh)
python3 "$D/mktap.py" $W/in.tap $W/in.tar:10240
: > $W/out.tap
P='# $'
printf '%s\t%s\t%s\n' \
 "" "login:" 600 \
 "root\r" "$P" 60 \
 "mkdir /tmp/cc$$; cd /tmp/cc$$; tar xpbf 20 /dev/rmt12\r" "$P" 120 \
 "sh cc.sh\r" "$P" 1800 \
 "\e" "sim> $" 20 \
 "det ts\r" "sim> $" 20 \
 "att ts $W/out.tap\r" "sim> $" 20 \
 "c\r" "" 2 \
 "\r" "$P" 20 \
 "tar cbf 20 /dev/rmt12 *.s\r" "$P" 300 \
 "sync\r" "$P" 30 > $W/steps
sed -e "s|^att ts .*|att ts $W/in.tap|" "$B/boot.ini" > $W/run.ini
(cd "$B" && python3 "$D/simhrun.py" . $W/run.ini $W/steps $W/log > $W/simh.out) || { cat $W/simh.out; exit 1; }
tr -d "\r" < $W/log | sed -n '/sh cc.sh/,/sim>/p' | grep -v "^C-68000\|getty\|sim>\|Simulation\|sh cc.sh" | grep -v "^myname# $" || true
python3 "$D/untap.py" $W/out.tap $W/out.tar > /dev/null
for f in "$@"; do tar -xf $W/out.tar -C "$(dirname "$f")" "$(basename "${f%.c}").s"; done
