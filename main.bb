new {
    value = 0;
    routes = std::server(8000);
    routes["/hi/<query>"] = {
        if((query as std::int(query))==false)
            return "The number of hi must be an integer.";
        if(query<=0)
            return "Need a positive number of hi. Why must you take them away? :-(";
        this.value = this.value + 1;
        return std::str(this.value+query)+" hello your "+std::str(query)+" hi";
    }
}

while(true) {}  // wait indefinitely