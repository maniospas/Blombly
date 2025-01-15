# IO

Blombly offers ways to interact with the file system and web resources.
*Expect more options in the future.*
In addition to the basic operations described here, find supporting 
functions in [libs](../advanced/libs.md).


## Permissions

If you run IO operations out-of-the-box, you will likely encounter an error
like below. This happens because Blombly prioritizes **execution safety** and does 
not allow you to access system resources unless you intent to do so. 
By default, the virtual machine only has read access rights for the *bb://libs* (the contents
of its standard library next to the executable) and working
directories. It cannot modify anything and cannot read from anywhere else.

<br>

This ensures that the one running the virtual machine is always in control of effects
on their machine. For example, this limits 
[preprocessor](../advanced/preprocessor.md) directives in included files from escaping
from the intended build system. Normal safety features from your operating system also 
apply externally, but this is how Blombly ensures that its programs are as safe as they can get.

```java
// main.bb
f = file("../README.md");
print(f);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto;">
> <span style="color: cyan;">./blombly</span> main.bb
(<span style="color: red;"> ERROR </span>) Access denied for path: ../README.md
   <span style="color: yellow;">!!!</span> This is a safety measure imposed by Blombly.
       You need to add read permissions to a location containing the prefix with `!access "location"`.
       Permissions can only be granted this way from the virtual machine's entry point.
       They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.
   <span style="color: lightblue;">→</span>  file("README.md")                                   playground/main.bb line 1
</pre>

Blombly permissions can only be declared on the file that is directly
executed; they will create errors if declared elsewhere without already
being allowed.
There are two kinds of permissions; read access with the `!access @str` 
directive, and read and write access with the `!modify @str` directive.
Resource paths can only extend the provided strings, but multiple permissions
may be declared.

<br>

Permissions apply everywhere for the running language instance.
For example, they restrict include directives, and code executed during compilation.
But they do not carry over to new runs, for example of *.bbvm* files.
Those do not preserve permissions to let people running programs
protect themselves against unintended consequences. You

```java
// permissions.bb
// grants read permissions to the higher-level directory
!access "../"
```

```java
// main.bb
// permissions here are available only when compiling and running the .bb file
!access "../"
print("../README.md"|file|str|len);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
<span style="color:green;"># compilation without running</span>
> <span style="color: cyan;">./blombly</span> main.bb  --norun
<span style="color:green;"># option 1: directly run permission code</span>
> <span style="color: cyan;">./blombly</span> {!access "../"} main.bbvm
<span style="color:green;"># option 2: run permission file</span>
> <span style="color: cyan;">./blombly</span> permissions.bb main.bbvm
</pre>

## FIles


The main way blombly interacts with the file system is through the `file` data type.
This is an abstraction over resources like files, directories, and web data obtained
with http. Path meaning and resolution depends on their prefix, as described in the second
of the drop-down tables below.
Operations applicable to file data include pushing data to resources and iterating
through retrieved resource data are summarized here. Of those operations, pushing
is applicable only to files and overwrites their text contents with the pushed string.

<details>
  <summary>Operations</summary>
  <table>
    <thead>
      <tr>
        <th>Operation</th>
        <th>Description</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td>push</td>
        <td>Write data to the resource without expecting persistence.</td>
      </tr>
      <tr>
        <td>iter</td>
        <td>Traverse through resource contents (used by the in macro internally).</td>
      </tr>
      <tr>
        <td>list</td>
        <td>Obtain all contents of the resource at once.</td>
      </tr>
      <tr>
        <td>clear</td>
        <td>Clear the resource data if possible.</td>
      </tr>
      <tr>
        <td>bool</td>
        <td>Check whether the resource exists.</td>
      </tr>
      <tr>
        <td>Division by a string</td>
        <td>Obtain a sub-directory from the resource.</td>
      </tr>
    </tbody>
  </table>
</details>

<details>
  <summary>Special path prefixes</summary>
<table>
  <thead>
    <tr>
      <th>Prefix</th>
      <th>Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>http://</td>
      <td>Refers to a resource accessible over the HTTP protocol.</td>
    </tr>
    <tr>
      <td>https://</td>
      <td>Refers to a resource accessible over the HTTPS protocol.</td>
    </tr>
    <tr>
      <td>ftp://</td>
      <td>Refers to a files accessible over the FTP protocol.</td>
    </tr>
    <tr>
      <td>ftps://</td>
      <td>Refers to a files accessible over the FTPS protocol.</td>
    </tr>
    <tr>
      <td>sftp://</td>
      <td>Refers to a files accessible over the SFTP protocol (FTP over SSH).</td>
    </tr>
    <tr>
      <td>bb://</td>
      <td>Starts from the directory where the Blombly executable resides.</td>
    </tr>
    <tr>
      <td>raw://</td>
      <td>Do not modify the provided path (this is not the default for safety).</td>
    </tr>
    <tr>
      <td>vfs://</td>
      <td>Accesses the path on a virtual file system. This is lost when the interpreter exits but persists throughout all compilation.</td>
    </tr>
    <tr>
      <td>Empty path</td>
      <td>Used to grant permissions for everything (NOT RECOMMENDED)</td>
    </tr>
    <tr>
      <td>No prefix</td>
      <td>Starts from the user's working directory.</td>
    </tr>
  </tbody>
</table>
</details>


Iterating through file contents yields the read lines. Whereas iterating
through directories yields their contents. You cannot push to directories, 
and -for safety- can only clear empty directories.
Here is an example for reading from the local file system,
as well as checking whether a non-existing file name exists:

```java
!access "" // read access to all resources (NOT RECOMMENDED)

f = file("README.md");
while(line in f) print(line);
print("nonexisting filename"|file|bool); // false
```

## Web resources

Web resources are read in the same way given the prefixes for the protocols:
http, https, ftp, sftp, ftps, sftp. The first two of those protocols do not
support pushing to the resource but the rest do.
All resource access errors can be intercepted and handled normally
with Blombly's `try-catch` pattern. 

```java
// main.bb
// grant necessary read access rights
!access "https://" 
start = time();

// join the string contents of the file iterable
response = "https://www.google.com"|file|bb.string.join("\n");

print("Response length: !{response|len}");
print("Response time: !{time()-start} sec");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb 
Response length: 55078 
Response time: 0.319413 sec 
</pre>

## Authentication

For web resources, you can set authentication and timeout parameters.
With the element setting notation. Once set, these parameters cannot
be retrieved and do not appear in any error messages.
Below is an example, in which set username and password with credentials from
[sftpcloud's](https://sftpcloud.io/tools/free-ftp-server) free SFTP server - create
a testing server for one hour with just a click and without logging in.

```java
!modify "sftp://"

// new server for 1 hour at: https://sftpcloud.io/tools/free-ftp-server
username = "username";
password = "password";

// create file from sftp
sftp = "sftp://eu-central-1.sftpcloud.io/test_file"|file;
sftp["username"] = username;
sftp["password"] = password; 
sftp["timeout"] = 10;
push(sftp, "Transferred data.");

// retrieve file from sftp
sftp = "sftp://eu-central-1.sftpcloud.io/test_file"|file;
sftp["username"] = username;
sftp["password"] = password; 
sftp["timeout"] = 10;
print(ftp|bb.string.join("\n"));
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb 
Transferred data.
</pre>



## Servers

Blombly offers the ability to set up REST services.
Instantiating a server is as simple as calling `routes=std::server(port)`,
where the port is provided. The server starts running immediately,
and you can dynamically add or remove routes from it. Execute client
request through the file system, as described above.

<br>

Treat the generated server as a map from resource location strings to code blocks
to be called when the respective resource is requested. Blocks that run this
ways should returned either a string plain text or a request result struct (see below). 
Parts of resource names that reside in angular brackets `<…>` indicate that the respective 
part of therequest should be treated as a string argument to the callable.
For example, the following snippet redirects `echo/<input>` to echo the provided input;
run code and open the browser and visit `localhost:8000/echo/MyTest` to see this in action.

```java
// main.bb
routes = server(8000);
routes["/echo/<input>"] => input; // equivalent to `... = {return input}`
while(true) {}  // wait indefinitely
```

In addition to parameters obtained by parsing the request, calls
to route code blocks may be enriched with status information, if available.
Related values that may be present are listed below.

<details>
  <summary>Status information</summary>
  <table style="border-collapse: collapse; width: 100%; margin-top: 10px;">
    <thead>
      <tr>
        <th>Value</th>
        <th>Type</th>
        <th>Details</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td>method</td>
        <td>str</td>
        <td>"GET" or "POST".</td>
      </tr>
      <tr>
        <td>content</td>
        <td>str</td>
        <td>Any received message content. This requires further parsing.</td>
      </tr>
      <tr>
        <td>ip</td>
        <td>str</td>
        <td>The user's IP address.</td>
      </tr>
      <tr>
        <td>http</td>
        <td>str</td>
        <td>The HTTP protocol's version.</td>
      </tr>
      <tr>
        <td>query</td>
        <td>str</td>
        <td>Any query parameters; those following after the question mark (?).</td>
      </tr>
      <tr>
        <td>ssl</td>
        <td>bool</td>
        <td>Whether HTTPS or WS is used (SSL/TLS used).</td>
      </tr>
      <tr>
        <td>uri</td>
        <td>str</td>
        <td>The request's full URI.</td>
      </tr>
    </tbody>
  </table>
</details>


Non-text results for server methods are declared by returning
a struct with text and type fields, like in the following example.

```java
content = "
    <!DOCTYPE html>
    <html>
        <head><title>Hi world!</title></head>
        <body><h1>Hi world!</h1>This is your website. 
        Add content in a <a href='https://perfectmotherfuckingwebsite.com/'>nice format</a>.</body>
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

Blombly wraps the [sqlite](https://www.sqlite.org/) database connector, which stores data in the file system.
Initialize a database with a string denoting a file location. An empty string creates a temporary
file to be deleted when closed, and `":memory:"` creates an in-memory instance without persistence.
Like before, databases require appropriate permissions to access the file system.
After initializing them, perform operations with the element access notation. Each operation returns list
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
        //db["DELETE FROM users WHERE id = {i};"]; // deletes are generally slow
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

!!! info
    Using brackets rather than parentheses to execute database queries
    lets us avoid side effects due to the potential concurrency of function
    or method calls that is intrinsic to Blombly. 
    To the contrary, built-in operations like element access run like normal
    sequential code, even if overloaded. This lets us write database queries
    one after the other and expect them to run in the same order.