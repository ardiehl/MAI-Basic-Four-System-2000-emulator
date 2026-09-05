#!/usr/bin/env python3
"""Read and edit the BOSS/IX configuration record inside a raw disk image.

The kernel refuses to boot when the record does not match what it expects, and
it says only "Invalid system configuration record" whichever of its five checks
failed, so editing the record by hand and getting the trailing checksum wrong
looks exactly like editing a field the machine dislikes. This does the
arithmetic, so a field change is a field change.

What the record is
------------------
The kernel reads two consecutive 512 byte blocks, 1024 bytes in all, and the
long at offset 0x3fc of that unit is a checksum over the preceding 1020 bytes.
The fields live in the second block, which is the 512 byte "configuration
record" that circulates on its own, so field offsets here are given relative to
that second block and the checksum offset relative to the unit.

Drive entries are packed into one 16 bit word: bits 0 to 10 are the size, bits
11 to 15 are how many drives of that size. A hard disk word of 0x1078 is two
drives of 120 MB. A floppy word of 0x0280 is no drives with a 640 KB capacity,
which is what a machine with the floppy disabled looks like, and 0x0a80 is one
640 KB drive.

Usage
-----
    configrec.py show <image>
    configrec.py set  <image> [--memory KB] [--floppies N] [--floppy-size KB]
                              [--disks N] [--disk-size MB] [--fourway N]
                              [--eightway N] [--parallel N] [--comm N]
                              [--name TEXT] [--block N]

The record is found by checksum rather than by a fixed block number, searching
from the end of the image towards the front because the record sits after the
last partition. --block overrides that.

Writing always recomputes the checksum. The image is modified in place, so work
on a copy.
"""

import argparse
import struct
import sys

BLOCK = 512
UNIT = 2 * BLOCK
CKSUM_OFF = 0x3fc          # in the unit
CKSUM_SEED = 0x14
REC = BLOCK                # the record is the second block of the unit

# offsets within the 512 byte record
F_USER_SSN = 0x100         # 8 ASCII digits
F_DISKS = 0x10c            # packed word, count and size in MB
F_MEMORY = 0x11c           # long, KB
F_FLOPPIES = 0x124         # packed word, count and size in KB
F_FOURWAY = 0x12c          # byte
F_SYSTYPE = 0x130          # word
F_REL_MINOR = 0x132        # byte, the 5 of 7.5B
F_REL_LETTER = 0x133       # byte, 1 is A, 2 is B
F_EIGHTWAY = 0x134         # byte
F_PARALLEL = 0x135         # byte
F_COMM = 0x136             # byte
F_CREATOR_SSN = 0x138      # 8 ASCII digits
F_NAME = 0x150             # NUL padded text
F_NAME_LEN = 32


def rol32(v, n=1):
    v &= 0xffffffff
    return ((v << n) | (v >> (32 - n))) & 0xffffffff


def checksum(unit):
    """Fold backwards over the 255 longs below the checksum itself."""
    acc = CKSUM_SEED
    p = CKSUM_OFF - 4
    while p >= 0:
        acc = rol32(acc ^ struct.unpack_from(">I", unit, p)[0], 1)
        p -= 4
    return acc


def unpack_drives(word):
    return (word >> 11) & 0x1f, word & 0x7ff


def pack_drives(count, size):
    if not 0 <= count <= 31:
        raise ValueError("drive count must be 0 to 31")
    if not 0 <= size <= 0x7ff:
        raise ValueError("drive size must be 0 to 2047")
    return (count << 11) | size


def looks_like_record(data, off):
    """Cheap test before paying for a checksum over 1020 bytes.

    The kernel compares seven bytes of the record against the serial number
    held in NVRAM, which is ASCII, so the serial number field has to be ASCII
    digits in any record the machine would accept.
    """
    ssn = data[off + REC + F_USER_SSN:off + REC + F_USER_SSN + 8]
    return len(ssn) == 8 and ssn.isdigit()


def find_record(data, prefilter=True):
    """The last unit in the image whose stored checksum matches its contents.

    Armin Diehl points out that the record sits after the last partition, so
    the search runs from the end of the image towards the front and stops at
    the first hit.
    """
    for blk in range(len(data) // BLOCK - 2, -1, -1):
        off = blk * BLOCK
        if prefilter and not looks_like_record(data, off):
            continue
        unit = data[off:off + UNIT]
        stored = struct.unpack_from(">I", unit, CKSUM_OFF)[0]
        if stored == 0:
            continue                      # an empty region checksums to zero
        if stored == checksum(unit):
            return blk
    return None


def text(unit, off, length):
    raw = bytes(unit[REC + off:REC + off + length])
    return raw.split(b"\0", 1)[0].decode("latin1")


def show(unit):
    ndisk, disksize = unpack_drives(struct.unpack_from(">H", unit, REC + F_DISKS)[0])
    nflop, flopsize = unpack_drives(struct.unpack_from(">H", unit, REC + F_FLOPPIES)[0])
    print("User SSN               %s" % text(unit, F_USER_SSN, 8))
    print("Creator SSN            %s" % text(unit, F_CREATOR_SSN, 8))
    print("System name            %s" % text(unit, F_NAME, F_NAME_LEN))
    print("System type            %d" % struct.unpack_from(">H", unit, REC + F_SYSTYPE)[0])
    print("Release level          7.%d%s" % (unit[REC + F_REL_MINOR],
                                             chr(ord("A") + unit[REC + F_REL_LETTER] - 1)))
    print("Main memory            %d KB" % struct.unpack_from(">I", unit, REC + F_MEMORY)[0])
    print("Hard disks             %d of %d MB" % (ndisk, disksize))
    print("Floppy drives          %d of %d KB" % (nflop, flopsize))
    print("4-Way controllers      %d" % unit[REC + F_FOURWAY])
    print("8-Way controllers      %d" % unit[REC + F_EIGHTWAY])
    print("Parallel ports         %d" % unit[REC + F_PARALLEL])
    print("Comm ports             %d" % unit[REC + F_COMM])
    print("Checksum               %08x" % struct.unpack_from(">I", unit, CKSUM_OFF)[0])


def edit(unit, args):
    changed = []
    if args.memory is not None:
        struct.pack_into(">I", unit, REC + F_MEMORY, args.memory)
        changed.append("memory %d KB" % args.memory)
    if args.floppies is not None or args.floppy_size is not None:
        n, size = unpack_drives(struct.unpack_from(">H", unit, REC + F_FLOPPIES)[0])
        if args.floppies is not None:
            n = args.floppies
        if args.floppy_size is not None:
            size = args.floppy_size
        struct.pack_into(">H", unit, REC + F_FLOPPIES, pack_drives(n, size))
        changed.append("floppies %d of %d KB" % (n, size))
    if args.disks is not None or args.disk_size is not None:
        n, size = unpack_drives(struct.unpack_from(">H", unit, REC + F_DISKS)[0])
        if args.disks is not None:
            n = args.disks
        if args.disk_size is not None:
            size = args.disk_size
        struct.pack_into(">H", unit, REC + F_DISKS, pack_drives(n, size))
        changed.append("hard disks %d of %d MB" % (n, size))
    for opt, off, label in ((args.fourway, F_FOURWAY, "4-Way"),
                            (args.eightway, F_EIGHTWAY, "8-Way"),
                            (args.parallel, F_PARALLEL, "parallel ports"),
                            (args.comm, F_COMM, "comm ports")):
        if opt is not None:
            if not 0 <= opt <= 255:
                raise ValueError("%s count out of range" % label)
            unit[REC + off] = opt
            changed.append("%s %d" % (label, opt))
    if args.name is not None:
        raw = args.name.encode("latin1")
        if len(raw) >= F_NAME_LEN:
            raise ValueError("system name must be under %d characters" % F_NAME_LEN)
        unit[REC + F_NAME:REC + F_NAME + F_NAME_LEN] = raw.ljust(F_NAME_LEN, b"\0")
        changed.append("name %r" % args.name)
    return changed


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("action", choices=("show", "set"))
    ap.add_argument("image")
    ap.add_argument("--block", type=int, help="first block of the 1024 byte unit")
    ap.add_argument("--memory", type=int, metavar="KB")
    ap.add_argument("--floppies", type=int, metavar="N")
    ap.add_argument("--floppy-size", type=int, metavar="KB")
    ap.add_argument("--disks", type=int, metavar="N")
    ap.add_argument("--disk-size", type=int, metavar="MB")
    ap.add_argument("--fourway", type=int, metavar="N")
    ap.add_argument("--eightway", type=int, metavar="N")
    ap.add_argument("--parallel", type=int, metavar="N")
    ap.add_argument("--comm", type=int, metavar="N")
    ap.add_argument("--name", metavar="TEXT")
    args = ap.parse_args()

    mode = "r+b" if args.action == "set" else "rb"
    with open(args.image, mode) as f:
        data = f.read()

        if args.block is not None:
            blk = args.block
        else:
            blk = find_record(data)
            if blk is None:
                # the cheap test rules out almost every block, so if it ruled
                # out all of them, pay for the slow pass before giving up
                blk = find_record(data, prefilter=False)
            if blk is None:
                sys.exit("no configuration record found: no block pair checksums correctly")

        off = blk * BLOCK
        unit = bytearray(data[off:off + UNIT])
        if len(unit) < UNIT:
            sys.exit("block %d is past the end of the image" % blk)
        if struct.unpack_from(">I", unit, CKSUM_OFF)[0] != checksum(unit):
            sys.exit("block %d does not hold a valid record" % blk)

        print("configuration record at blocks %d and %d" % (blk, blk + 1))
        if args.action == "show":
            show(unit)
            return

        changed = edit(unit, args)
        if not changed:
            sys.exit("nothing to change")
        struct.pack_into(">I", unit, CKSUM_OFF, checksum(unit))
        f.seek(off)
        f.write(bytes(unit))
        print("changed: %s" % ", ".join(changed))
        print()
        show(unit)


if __name__ == "__main__":
    main()
