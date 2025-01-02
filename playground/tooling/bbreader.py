allcode = list()
tab = 0
path = "main.bbvm"
anons = [dict()]
with open(path) as file:
    for line in file:
        line = line[:-1].split(' ')
        if line[0] == "END":
            tab -= 1
            allcode.append((tab*"   ")+"}")
            #anons.pop()
            continue
        if line[1] != "#":
            code = line[1] + " = "
        else:
            code = ""
        if line[0] == "BEGIN":
            code += "{"
            allcode.append(tab*"   "+code)
            tab += 1
            #anons.append(dict())
            continue
        elif line[0] == "BUILTIN":
            if line[2][0] == "\"":
                code += " ".join(line[2:])+";"
            else:
                code += line[2][1:]+";"
        elif line[0] == "BEGINFINAL":
            code = "final "+code+"{"
            allcode.append(tab*"   "+code)
            tab += 1
            continue
            #anons.append(dict())
        elif line[0] == "copy":
            code += line[2]+";"
        elif line[0] == "inline":
            code += line[2]+":"
        elif line[0] == "get":
            code += line[2]+"."+line[3]+";"
        elif line[0] == "set":
            code += line[2]+"."+line[3]+" = "+line[4]+";"
        elif line[0] == "final":
            for j in range(len(allcode)-1, 0, -1):
                if allcode[j].startswith(code):
                    allcode[j] = (" "*(len(allcode[j])-len(allcode[j].lstrip())))+"final "+allcode[j].lstrip()
                    break
            continue
        else:
            code += line[0] + "("+(",".join(line[2:]))+");" 
        allcode.append((tab*"   ")+code)


print("============ BBVM Reader =============")
print("  Reverse engineering source code\n  from blombly VM assembly")
print("  Author: Emmanouil Krasanakis")
print("  Email : maniospas@hotmail.com")
print("============== Details ===============")
print(f"  Analysed: {path}")
print("============== Source ================")
print("\n".join(allcode))
print("======================================")