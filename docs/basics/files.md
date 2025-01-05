# File system

The main way blombly interacts with the file system is through the `file` data type.
This is an abstraction over resources like files, directories, and web data obtained
with http. These are automatically recognized given the provided path.

Operations applicable to data are:
- `push` data to write to the resource, though do not expect persistence.
- `iter` to traverse through resoucre contents (and the `in` macro that uses iterators under the hood).
- `list` to obtain all conntents at once.
- `clear` to clear resource data if possible.
- `bool` to check whether the resource exists.
- division by a string to obtain a sub-directory.


## Reading files

File contents are organized into lines.
Here is an example for reading from the local file system,
as well as checking whether a non-existing file name exists:

```java
f = file("README.md");
while(line in f) print(line);
print("nonexisting filename"|file|bool); // false
```

## Reading directories

You can list the file paths of a directory. Paths can be added to resources,
but the original file location cannot be retrieved.
*This lets paths remain confidential once first declared.*
For example, one cannot expose local directory structure when running servers
or printing logs, unless this information is explicitly retained in code,
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

while(path in files("src")) print(path);
```

You cannot push to directories, and -for safety- can only clear empty directories.
Use functions like the above to clear files and directories.


## Reading web resources

Web resources are accessed in the same way. Here is an example:

```java
get(url) = {
    ret = "";
    while(line in url|file) ret += line;
    return ret;
}
start = time();
print("Waiting");

response = get("https://www.google.com");
print("Response length: {response|len}");
print("Response time: {time()-start} sec");
```


