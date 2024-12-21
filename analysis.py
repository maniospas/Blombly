def analyze_block(lines: list[str], start: int, end: int, tab: str=""):
    i: int = start
    inputs = set()
    outputs = set()
    blocks = dict()
    types = dict()

    def memset(name):
        #if name not in inputs:
        if name == "#":
            return
        outputs.add(name)

    def memget(name):
        if name not in outputs:
            inputs.add(name)

    def inline(block):
        inputs, outputs = block
        for variable in inputs:
            memget(variable)
        for variable in outputs:
            memset(variable)

    while i<=end:
        line = lines[i]
        command = line[0]
        for argument in line[2:]:
            memget(argument)
        if len(line) > 1:
            memset(line[1])
        if command == "BEGIN" or command == "BEGINFINAL":
            blockname = line[1]
            types[blockname] = "block"
            i += 1
            blockstart = i
            depth = 1
            while i<=end:
                if lines[i][0] == "BEGIN" or lines[i][0] == "BEGINFINAL":
                    depth += 1
                elif lines[i][0] == "END":
                    depth -= 1
                if depth == 0:
                    blockend = i
                    blocks[blockname] = analyze_block(lines, blockstart, blockend, tab)
                    print(tab, f"{blockname}")
                    print(tab, "  Inputs", blocks[blockname][0])
                    print(tab, "  Outputs", blocks[blockname][1])
                    i += 1
                    break
                i += 1
            continue
        i += 1
    return inputs, outputs




with open("main.bbvm") as file:
    lines = [line[:-1].split(" ") for line in file]

analyze_block(lines, 0, len(lines)-1)