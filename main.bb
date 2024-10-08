final hello = { // code block that can be called or inlined elsewhere
    #spec author = "maniospas"; // any spec field can be set
    #spec version = "v1.0.0"; // can set field to objects too
    print("Hello "+name+"!"); 
}

print(hello.version);
name = read("What's your name?");
hello(name=name);