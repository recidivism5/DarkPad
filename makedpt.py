import sys,struct
with open(sys.argv[1],"r") as i:
    with open(sys.argv[2],"wb") as o:
        lines = i.read().split('\n')
        d = 0
        if lines[0].split(':')[0] == "TitlebarIcon":
            o.write(lines[0].split(':')[1].encode("ascii"))
            d = 1
        o.write(struct.pack("b",0))
        for n in range(d,len(lines),1):
            o.write(struct.pack("<I",int(lines[n].split(':')[1],16)))