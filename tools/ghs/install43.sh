#!/bin/bash
# Install stock 4.3BSD on the Open SIMH VAX-11/780, the host for the Green
# Hills cross compilers. Put stand.gz, miniroot.gz, rootdump.gz and usr.tar.gz
# from the TUHS 4.3BSD directory and boot42 (Computer History Wiki, page
# "Boot42") into $GHS_WORK/bsd43 first.
#   SIMH=/path/to/vax780 ./install43.sh

set -e
D=$(cd "$(dirname "$0")" && pwd)
W=${GHS_WORK:-$D/work}/bsd43
export SIMH=${SIMH:-vax780}
cd "$W"
for f in stand miniroot rootdump usr.tar; do [ -f $f ] || gzip -dc $f.gz > $f; done
python3 "$D/mktap.py" 43.tap stand:512 miniroot:10240 rootdump:10240 usr.tar:10240
cp miniroot miniroot.dsk
: > rq.dsk

common='set rq2 dis
set rq3 dis
set rp dis
set lpt dis
set rl dis
set tq dis
set tu dis
att ts 43.tap
set tti 7b
set tto 7b
load -o boot42 0
d r10 9
d r11 0
run 2'
printf 'set rq0 ra81\nat rq0 miniroot.dsk\nset rq1 ra81\nat rq1 rq.dsk\n%s\n' "$common" > install.ini
printf 'set rq0 ra81\natt rq0 rq.dsk\nset rq1 dis\n%s\n' "$common" > boot.ini

P='\n# $'
printf '%s\t%s\t%s\n' \
 "" "$P" 180 \
 "cd /dev\r" "$P" 30 \
 "./MAKEDEV ra1\r" "$P" 60 \
 "cd /\r" "$P" 30 \
 "disk=ra1 type=ra81 tape=ts xtr\r" "extracted(.|\n)*# $" 900 \
 "sync\r" "$P" 30 > phase1.steps
python3 "$D/simhrun.py" . install.ini phase1.steps phase1.log

printf '%s\t%s\t%s\n' \
 "" "$P" 300 \
 "name=ra0h;type=ra81\r" "$P" 20 \
 "cd /dev; sh ./MAKEDEV ts0 dz0; sync; cd /\r" "$P" 60 \
 "newfs \$name \$type\r" "$P" 600 \
 "mount /dev/\$name /usr; cd /usr; mt rew; mt fsf 3\r" "$P" 120 \
 "tar xpbf 20 /dev/rmt12\r" "$P" 1800 \
 "cd /; chmod 755 / /usr; umount /dev/\$name\r" "$P" 60 \
 "fsck /dev/r\$name\r" "$P" 600 \
 "cp /etc/fstab.ra81 /etc/fstab\r" "$P" 20 \
 "newfs ra0g ra81\r" "$P" 900 \
 "sync; sync\r" "$P" 30 > phase2.steps
python3 "$D/simhrun.py" . boot.ini phase2.steps phase2.log
echo "4.3BSD installed in $W/rq.dsk"
