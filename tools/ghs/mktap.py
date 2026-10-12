import struct, sys

# Build a SIMH tape image from (file, record size) pairs, like mkdisttap.pl.
#   mktap.py out.tap file:recsize ...
out = open(sys.argv[1], "wb")


def rec(data):
    n = struct.pack("<I", len(data))
    out.write(n + data + (b"\0" if len(data) & 1 else b"") + n)


for spec in sys.argv[2:]:
    name, size = spec.rsplit(":", 1)
    size = int(size)
    data = open(name, "rb").read()
    for i in range(0, len(data), size):
        chunk = data[i:i + size]
        if len(chunk) < size:
            chunk += b"\0" * (size - len(chunk))
        rec(chunk)
    out.write(struct.pack("<I", 0))
out.write(struct.pack("<I", 0))
out.close()
