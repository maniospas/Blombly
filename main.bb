isfolder(path) => bool(path+"/.");

list_contents(file path) = {
    print(path);
    default tab = "";
    while(subpath in path|file)
        print(subpath, subpath|isfolder);
        //if(subpath|isfolder)
        //    list_contents(subpath :: tab="{tab}  ");
}
list_contents("src");