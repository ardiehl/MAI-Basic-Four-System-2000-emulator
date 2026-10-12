import os, pty, select, sys, time, signal, re

# Boot the single user image to ADMIN> and run commands; an argument of the
# form @name=localfile types that local file into name with cat and ^D.
#   emusess.py <emulator dir> <disk image> cmd|@name=file ...
emudir, img = sys.argv[1], sys.argv[2]
os.chdir(os.path.expanduser(emudir))
pid, fd = pty.fork()
if pid == 0:
    os.execv("./eagleemu", ["./eagleemu", "msg all -", "bus -", "msg all +err",
                            "dev wd image " + img] + [c for c in os.environ.get("EXTRA", "").split(";") if c] + ["g"])
out = bytearray()
tail = b""


def pump(sec):
    global tail
    end = time.time() + sec
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                d = os.read(fd, 65536)
            except OSError:
                return
            if not d:
                return
            out.extend(d)
            tail = (tail + d)[-8000:]


def typ(s):
    if os.environ.get("FAST"):
        for i in range(0, len(s), 64):
            os.write(fd, s[i:i + 64].encode("latin1"))
            pump(0.01)
        return
    for ch in s:
        os.write(fd, ch.encode("latin1"))
        pump(0.02)


def clean(b):
    b = b.decode("latin1").replace("\r", "")
    return re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]", "", b)


resp = [("Boot device:", "wd0\r"), ("System file:", "\r"),
        ("begin automatic", "\r"), ("Update clock", "\r"),
        ("key to continue", "\x1b")]
t0 = time.time()
while time.time() - t0 < 200 and b"ADMIN>" not in tail:
    pump(0.3)
    for p, rp in resp:
        if p.encode() in tail:
            time.sleep(0.5)
            os.write(fd, rp.encode())
            tail = b""
            break

limit = float(os.environ.get("LIMIT", "60"))
for a in sys.argv[3:]:
    tail = b""
    mark = len(out)
    if a == "!shutdown":
        os.write(fd, b"\x04")
        t = time.time()
        while time.time() - t < 60 and b"shutdown?" not in tail:
            pump(0.2)
        time.sleep(0.5)
        typ("shutdown")
        os.write(fd, b"\r")
        t = time.time()
        while time.time() - t < 120 and b"to reboot" not in tail:
            pump(0.3)
        print("=== shutdown: %s" % ("ok" if b"to reboot" in tail else "NO LLEGO"))
        continue
    if a.startswith("@"):
        name, path = a[1:].split("=", 1)
        typ("cat > " + name)
        os.write(fd, b"\r")
        pump(1)
        for line in open(os.path.expanduser(path)).read().splitlines():
            typ(line)
            os.write(fd, b"\r")
            pump(0.1)
        os.write(fd, b"\x04")
        cmd = "(cat > %s)" % name
    else:
        typ(a)
        os.write(fd, b"\r")
        cmd = a
    t = time.time()
    while time.time() - t < limit:
        pump(0.2)
        if b"ADMIN>" in tail and time.time() - t > 1:
            break
        if os.environ.get("PROMPT2") and tail.endswith(os.environ["PROMPT2"].encode()) and time.time() - t > 0.2:
            break
    body = clean(bytes(out[mark:]))
    if os.environ.get("QUIET") and not a.startswith(("/", "ls", "filemodes")):
        continue
    print("=== %s   (%.1f s)" % (cmd, time.time() - t))
    if not a.startswith("@"):
        for l in body.splitlines()[1:]:
            if l.strip() and l.strip() != "ADMIN>":
                print("   " + l[:130])
os.kill(pid, signal.SIGKILL)
