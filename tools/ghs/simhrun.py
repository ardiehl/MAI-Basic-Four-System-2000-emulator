import os, pty, select, sys, time, re

# Run vax780 with an ini file and play a step file, one step per line:
#   send<TAB>wait regex<TAB>timeout seconds
# send uses \r, \e (Ctrl-E, back to sim>) escapes; an empty send only waits.
#   simhrun.py <dir> <ini> <steps> [log]
wd, ini, steps = sys.argv[1:4]
log = sys.argv[4] if len(sys.argv) > 4 else None
os.chdir(os.path.expanduser(wd))
pid, fd = pty.fork()
if pid == 0:
    os.execvp(os.environ.get("SIMH", "vax780"), ["vax780", ini])
out = bytearray()
tail = ""


def pump(sec):
    global tail
    end = time.time() + sec
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                d = os.read(fd, 65536)
            except OSError:
                return False
            if not d:
                return False
            out.extend(d)
            tail = (tail + d.decode("latin1").replace("\r", ""))[-6000:]
    return True


t0 = time.time()
ok = True
for line in open(steps):
    line = line.rstrip("\n")
    if not line or line.startswith("#!"):
        continue
    send, wait, tmo = line.split("\t")
    tail = ""
    if send:
        send = send.replace("\\r", "\r").replace("\\e", "\x05")
        for ch in send:
            os.write(fd, ch.encode())
            pump(0.02)
    t = time.time()
    hit = False
    while time.time() - t < float(tmo):
        if not pump(0.3):
            break
        if re.search(wait, tail):
            hit = True
            break
    print("%-6.0f %-45s -> %s (%.0f s)" % (time.time() - t0, send.strip()[:45],
          "ok" if hit else "NO LLEGO " + repr(wait), time.time() - t), flush=True)
    if not hit:
        print("   cola: %r" % tail[-400:])
        ok = False
        break
if log:
    open(log, "wb").write(bytes(out))
try:
    os.write(fd, b"\x05")
    pump(1)
    os.write(fd, b"q\r")
    pump(2)
except OSError:
    pass
sys.exit(0 if ok else 1)
