new {
    value = 0;
    routes = std::server(8000);
    routes["/"] = {
        this.value = this.value + 1;
        return "hi " + std::str(this.value)+" to your "+query;
    }
}

while(true) {}  // wait indefinitely