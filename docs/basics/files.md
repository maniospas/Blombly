# File system

The main way blombly interacts with the file system is through the `file` data type.
This is an abstraction over resources like files, directories, and web data obtained
with http. These are automatically recognized given the provided path.

## Overview

Operations applicable to file data are:

- `push` data to write to the resource, though do not expect persistence.
- `iter` to traverse through resoucre contents (and the `in` macro that uses iterators under the hood).
- `list` to obtain all conntents at once.
- `clear` to clear resource data if possible.
- `bool` to check whether the resource exists.
- division by a string to obtain a sub-directory.

Blombly executes as a compartmenized environment. Therefore, for any of the above operations to run, you need to declare ...

## Permissions

If you try to run file system operations out-of-the-box you will encounter an error
like below. This happens because Blombly prioritizes **execution safety** and does 
not allow you to access system resources unless you intent to do so. Intent is obvious
here, but may not be if [preprocessor](../advanced/preprocessor.md) directives are involved,
like including code that uses `!comptime` for dependency retrieval. 
Normal operating system safety features also apply externally.

```java
// main.bb
f = file("README.md");
print(f);
```

```text
> ./blombly playground/main.bb
( ERROR ) Access denied for path: README.md
   !!! This is a safety measure imposed by Blombly.
       You need to add read permissions to a location containting the prefix with `!access "location"`.
       Permisions can only be granted this way from the virtual machine's entry point.
       They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.
   → file("README.md")                                   playground/main.bb line 1
Docs and bug reports: https://maniospas.github.io/Blombly
```

Blombly permissions can only be declared on the file that is directly
executed; they will create errors if declared elsewhere but don't have access.
There are two kinds of permissions; read access with the `!access @str` 
directive and read&write access with the `!modify @str` directive.

Both directives parse a string that indicates the *beginning* of an accepted file path.
You may grant multiple persissions too. Below is an example of granting permissions to accessing
an *https* and writting on a local directory. Permissions extend everywhere, 
including to files being included and `!comptime`. 


```java
!access "https://raw.githubusercontent.com/"
!modify "libs/download/"

bb.os.transfer("libs/download/html.bb", "https://raw.githubusercontent.com/maniospas/Blombly/refs/heads/main/libs/html.bb");
```

## Usage

File contents are organized into lines.
Here is an example for reading from the local file system,
as well as checking whether a non-existing file name exists:

```java
!access "" // read access to every file in your system and the nextwork (NOT RECOMMENDED)

f = file("README.md");
while(line in f) print(line);
print("nonexisting filename"|file|bool); // false
```

As an example, list the paths of a directory with the code below. 
You cannot push to directories, and -for safety- can only clear empty directories.
Use functions like the above to clear files and directories.

```java
!access ""  // read access to every file in your system and the nextwork (NOT RECOMMENDED)

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

Web resources are accessed in the same way. Here is an example:

```java
!access "http://"  // allow http requests
!access "https://" // also allow https requests

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


## Standard library support

*This section is under construction.*