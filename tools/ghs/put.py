import sys

# Print the BASIC statements that recreate a host file inside BOSS/IX, one
# argument per statement, for emusess.py.
#   put.py <local file> <target path> [bytes per line]
src, dst = sys.argv[1], sys.argv[2]
n = int(sys.argv[3]) if len(sys.argv) > 3 else 60
data = open(src, "rb").read()
cmds = ["basic", 'STRING "%s"' % dst, 'OPEN (1)"%s"' % dst]
for i in range(0, len(data), n):
    cmds.append('X$=ATH("%s")' % data[i:i + n].hex().upper())
    cmds.append("WRITE RECORD(1)X$")
cmds += ["CLOSE (1)", "RELEASE"]
print("\n".join(cmds))
