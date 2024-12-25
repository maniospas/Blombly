final isfolder(file path) => bool(path/".");
final files(file path) = {
    default tab = "";
    ret = list();
    while(subpath in path|file) try {
        if(subpath=="." or subpath=="..") return;
        if(subpath|isfolder) ret += files(subpath :: tab="{tab}  ") else push(ret, subpath);
    }
    return ret;
}

while(path in files("src"))
    print(path);
