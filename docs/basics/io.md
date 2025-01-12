# IO

Blombly offers several input and output operations.
*The ones below are currently implemented, and more will become available in the future.*
In addition to the basic operations described here, find supporting ones in [libs](../advanced/libs.md).


## File system

The main way blombly interacts with the file system is through the `file` data type.
This is an abstraction over resources like files, directories, and web data obtained
with http. These are automatically recognized given the provided path. 
Operations applicable to file data are summarized here.

| **Operation**         | **Description**                                                                          |
|------------------------|------------------------------------------------------------------------------------------|
| `push`                | Write data to the resource without expecting persistence.                                 |
| `iter`                | Traverse through resource contents (used by the `in` macro internally).                   |
| `list`                | Obtain all contents of the resource at once.                                              |
| `clear`               | Clear the resource data if possible.                                                      |
| `bool`                | Check whether the resource exists.                                                        |
| Division by a string  | Obtain a sub-directory from the resource.                                                 |


If you run file system operations out-of-the-box you will encounter an error
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

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto;">
<span style="color: cyan;">> ./blombly</span> playground/main.bb
(<span style="color: red;"> ERROR </span>) Access denied for path: README.md
   <span style="color: yellow;">!!!</span> This is a safety measure imposed by Blombly.
       You need to add read permissions to a location containing the prefix with `!access "location"`.
       Permissions can only be granted this way from the virtual machine's entry point.
       They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.
   <span style="color: lightblue;">→</span>  file("README.md")                                   playground/main.bb line 1
</pre>

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

Iterating through file contents yields the read lines. Whereas iterating
through directories yields their contents. You cannot push to directories, and -for safety- can only clear empty directories.
Here is an example for reading from the local file system,
as well as checking whether a non-existing file name exists:

```java
!access "" // read access to every file in your system and the nextwork (NOT RECOMMENDED)

f = file("README.md");
thisdir = ".";
print("Is directory: {path/thisdir|bool}");
while(line in f) print(line);
print("nonexisting filename"|file|bool); // false
```

Web resources are accessed in the same way. 
Under the hood, they perform get requests. Here is an example:

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



## Servers

Blombly offers the ability to set up REST services.
Instantiating a server is as simple as calling `routes=std::server(port)`,
where the port is provided. The server starts running immediately,
and you can dynamically add or remove routes from it. Execute client
request through the file system, as desribed above.

<br>

Treat the generated server as a map from resource location strings to code blocks
to be called when the respective resource is requested. Blocks that run this
ways should returned either a string plain text or a request result struct (see below). 
Parts of resource names that reside in angular brackets `<...>` indicate that the respective 
part of therequest should be treated as a string argument to the callable.
For example, the following snippet redirects `echo/<input>` to echo the provided input;
run code and open the browser and visit `localhost:8000/echo/MyTest` to see this in action.

```java
// main.bb
routes = server(8000);
routes["/echo/<input>"] => input; // equivalent to `... = {return input}`
while(true) {}  // wait indefinitely
```

In addition to the keyword argument values obtained by parsing the request, calls
to route code blocks may be enriched with several positional arguments, if available.
These are listed below:

| Argument | Type | Details |
| -------- | ---- | ----------- |
| method   | str  | "GET" or "POST". |
| content  | str  | Any received message content. This requires further parsing. |
| ip       | str  | The user's ip address. |
| http     | str  | The http protocol's version. |
| query    | str  | Any query parameters; those following after the questionmark (`?`). |
| ssl      | bool | Whether HTTPS or WS is used (SSL/TLS used). |
| uri      | str  | The request's full uri. |


```java
// main.bb
new {
    value = 0;
    routes = server(8000);
    routes["/hi/<number>"] = {
        if(not number as int(number)) return "The number of hi must be an integer.";
        if(number<=0) return "Need a positive number of hi. Why must you take them away? :-(";
        this.value += 1;
        return "{this.value+number} hello your to {number} hi";
    }
}

print("Give me some greetings at localhost:8000/hi/<number>");
while(true) {}  // wait indefinitely
```

Non-text results for server methods are declared by returning
a struct with text and type fields, like below.

```java
content = "
    <!DOCTYPE html>
    <html>
        <head><title>Hi world!</title></head>
        <body><h1>Hi world!</h1>This is your website. Add content in a <a href='https://perfectmotherfuckingwebsite.com/'>nice format</a>.</body>
    </html>";
    
routes = server(8000);
routes[""] => new {
    str => this..content; // definition time closure
    type = "text/html";
}

print("Server running at http://localhost:8000/");
while(true){}
```

## Databases

Blombly includes the [sqlite](https://www.sqlite.org/) database implementation. This stores data in the file system.
Working with this database consists of initializing it on a file string, where an empty string creates a temporary
file to be deleted when closed and `":memory:"`to create and an in-memory database without persistence. Like with files,
databases require necessary permissions. Perform database operations with the access notation. Operations return list
data containing maps from column names to string values. Below is an example that iterates through a list of users.


```java
db = sqlite(":memory:");
db["PRAGMA journal_mode = 'wal';"]; // often speeds things up (https://www.sqlite.org/wal.html)
db["CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, age INTEGER);"];

while(i in range(5)) {
    db["BEGIN TRANSACTION;"]; // instead of this, prefer batching operations into larger transactions
        db["INSERT INTO users (name, age) VALUES ('User{i}', {20 + (i % 10)});"];
        db["SELECT * FROM users WHERE id = {i};"];
        db["UPDATE users SET age = age + 1 WHERE id = {i};"];
        //db["DELETE FROM users WHERE id = {i};"]; // deletes are generally very slow
    db["COMMIT;"];
}

while(user in db["SELECT * FROM users;"]) print(user);
db["DELETE FROM users;"];
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
{name: User0, age: 21, id: 1} 
{name: User1, age: 22, id: 2} 
{name: User2, age: 23, id: 3} 
{name: User3, age: 24, id: 4} 
{name: User4, age: 24, id: 5} 
</pre>
