# File system

The main way blombly interacts with the file system is through the `file` data type.
This is an abstraction over files, directories, and web resources.

## Reading files

Once created, files are read-only and provide iterators and conversion to string 
representations of their contents. Here is an example for reading a file from the local file system:

```java
f = file("README.md");
while(line in f)
    print(line);
print("nonexisting filename"|file|bool); // false
```

## Reading directories

You can similarly list the file paths of a directory. Paths can be added to files,
but the original file location cannot be retrieved.
*This lets paths remain confidential once first declared.*
For example, one cannot expose local directory structure when running servers
or printing logs, unless this information is explicitly retained
or leaked in file data.

```java
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
```

## Reading web resources

Web resources are accessed in the same way. Results or read disk data are cached,
so a new file needs to be created every time data need to be reread (from the web
or from disk). Here is an example:


```java
get(url) => url|file|str;  // equivalent to str(file(url))

start = bbvm::time();
print("Waiting");

response = get("https://www.google.com");
print("Response length: {response|len}");
print("Response time: {bbvm::time()-start} sec");
```


