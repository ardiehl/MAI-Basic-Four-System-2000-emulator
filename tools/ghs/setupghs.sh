#!/bin/bash
# Copy the Green Hills VAX compilers and the C library sources from the
# c68_p68_libr_1.8.3A tape (extracted) into the 4.3BSD VAX, as /vax and /LIB.
#   GHS_TAPE=/path/to/extracted/c68_p68_libr_1.8.3A_3-28-88 ./setupghs.sh
set -e
D=$(cd "$(dirname "$0")" && pwd)
W=${GHS_WORK:-$D/work}/bsd43
export SIMH=${SIMH:-vax780}
[ -d "$GHS_TAPE/BIN/vax" ] || { echo "GHS_TAPE: no BIN/vax in '$GHS_TAPE'"; exit 1; }
T=$(mktemp -d)
cp -r "$GHS_TAPE/BIN/vax" "$GHS_TAPE/LIB" $T/
(cd $T && tar cf ghs.tar vax LIB)
python3 "$D/mktap.py" "$W/ghs.tap" $T/ghs.tar:10240
sed -e "s|^att ts .*|att ts ghs.tap|" "$W/boot.ini" > "$W/ghs.ini"
P='# $'
printf '%s\t%s\t%s\n' \
 "" "login:" 600 \
 "root\r" "$P" 60 \
 "cd /; tar xpbf 20 /dev/rmt12\r" "$P" 300 \
 "ls -l /vax/compiler\r" "$P" 20 \
 "sync\r" "$P" 30 > "$W/ghs.steps"
(cd "$W" && python3 "$D/simhrun.py" . ghs.ini ghs.steps ghs.log)
tr -d '\r' < "$W/ghs.log" | sed -n '/ls -l/,$p' | grep ccom68
