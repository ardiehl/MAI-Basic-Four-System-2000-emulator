#!/usr/bin/env python3
# Pack files into a 640 KB floppy image for eagleemu, each one starting on a
# 512 byte block, and print the BOSS/IX debe commands that copy them out of
# /dev/fd0 again with their exact size. debe's iskip and oskip are byte
# offsets whatever bs is; whole blocks go with bs=512, which is much faster
# than bs=1, and the last partial block with bs=1, appended with -notrunc.
#   fdput.py <image> <local file>=<BOSS/IX path> ...
import sys

img = bytearray(640 * 1024)
pos = 0
for a in sys.argv[2:]:
    src, dst = a.split('=', 1)
    data = open(src, 'rb').read()
    if pos + len(data) > len(img):
        sys.exit('does not fit: ' + src)
    img[pos:pos + len(data)] = data
    full, rest = divmod(len(data), 512)
    if full:
        print('debe if=/dev/fd0 of=%s bs=512 iskip=%d count=%d' % (dst, pos, full))
    if rest:
        print('debe if=/dev/fd0 of=%s bs=1 iskip=%d oskip=%d count=%d%s' %
              (dst, pos + full * 512, full * 512, rest, ' -notrunc' if full else ''))
    pos += (len(data) + 511) // 512 * 512
open(sys.argv[1], 'wb').write(img)
