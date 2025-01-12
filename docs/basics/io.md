# IO

Blombly offers several input and output operations.
*The ones below are currently implemented, and more will become available in the future.*
In addition to the basic operations described here, find supporting ones in [libs](../advanced/libs.md).


## File system

The main way blombly interacts with the file system is through the `file` data type.
This is an abstraction over resources like files, directories, and web data obtained
with http. These are automatically recognized given the provided path. 
Operations applicable to file data include pushing data to resources and iterating
through retrieved resource data are summarized here.

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
> <span style="color: cyan;">./blombly</span> playground/main.bb
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
Permissions extend everywhere, including to files being included, the `!comptime` preprocessor directive that 
executes some code during compilation, as well as the generated *.bbvm* file. 

<br>

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

Blombly supports [sqlite](https://www.sqlite.org/) database, which store data in the file system.
Initialize the database on a file string, where an empty string creates a temporary
file to be deleted when closed, and `":memory:"` creates an in-memory instance without persistence. Like with files,
databases require permissions. Perform operations with the element access notation. Operations return list
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
