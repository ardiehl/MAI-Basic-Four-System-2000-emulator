import struct, subprocess, sys, time

# Turn a linked m68k ELF (text at 0, data at 0x400000) into a BOSS/IX .rel
# executable. Header as in /bin/link: magic, header size, text, data, bss, 0,
# symbols, relocation, 0, data base, bss base, 0, time, 1, 0, 0. The time is
# in seconds since 1 January 1980, local time.
#   mkrel.py prog.elf prog.rel
elf, out = sys.argv[1:3]
sec = {}
for l in subprocess.run(["m68k-linux-gnu-objdump", "-h", elf], capture_output=True,
                        text=True).stdout.splitlines():
    f = l.split()
    if len(f) >= 6 and f[1] in (".text", ".data", ".bss"):
        sec[f[1]] = (int(f[2], 16), int(f[3], 16))
def dump(name):
    return subprocess.run(["m68k-linux-gnu-objcopy", "-O", "binary", "-j", name, elf,
                           "/dev/stdout"], capture_output=True).stdout
text = dump(".text")
dbase = 0x400000
data = dump(".data") if ".data" in sec else b""
bsize, bbase = 0, dbase + len(data)
if ".bss" in sec:
    bsize, bbase = sec[".bss"]
    data += b"\0" * (bbase - dbase - len(data))   # pad data up to the bss
if len(text) & 1: text += b"\0"
if len(data) & 1: data += b"\0"
hdr = b".rel" + struct.pack(">15I", 0x40, len(text), len(data), bsize, 0, 0, 0,
                             0, dbase, dbase + len(data), 0,
                             int(time.time()) + time.localtime().tm_gmtoff - 315532800, 1, 0, 0)
open(out, "wb").write(hdr + text + data)
print("%s: text %d data %d bss %d" % (out, len(text), len(data), bsize))
