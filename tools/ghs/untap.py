import struct, sys
# Concatenate the records of the first file of a SIMH tape image.
d = open(sys.argv[1], "rb").read()
out = bytearray(); i = 0
while i + 4 <= len(d):
    n = struct.unpack("<I", d[i:i+4])[0]; i += 4
    if n == 0 or n == 0xffffffff: break
    n &= 0xffffff
    out += d[i:i+n]; i += n + (n & 1) + 4
open(sys.argv[2], "wb").write(out)
print(len(out))
