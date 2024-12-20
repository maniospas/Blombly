# Servers

Blombly offers the ability to set up REST services.
Instantiating a server is as simple as calling `routes=std::server(port)`,
where the port is provided. The server starts running immediately,
and you can dynamically add or remove routes from it. You can execute 
client requrests to online serves through blombly's [file system](../basics/files.md).


## Routes

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
routes["/echo/<input>"] = {return input;}
while(true) {}  // wait indefinitely
```

## Request input data

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
        if((number as int(number))==false)
            return "The number of hi must be an integer.";
        if(number<=0)
            return "Need a positive number of hi. Why must you take them away? :-(";
        this.value = this.value + 1;
        return str(this.value+number)+" hello your "+str(number)+" hi";
    }
}

print("Give me some greetings at localhost:8000/hi/<number>");
while(true) {}  // wait indefinitely
```

## Non-text results

*This segment is under constructions.*