routes = server(5000);
routes["/"] = {
    return "<!DOCTYPE html>
    <html>
        <body>
        <h1>My First Heading</h1>
        <p>My first paragraph.</p>
        </body>
    </html>"
}
while(true) {}  // wait indefinitely
