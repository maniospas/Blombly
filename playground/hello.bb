g = graphics("MyApp", 800, 600);

while(events as g|pop) {
    while(event in events) {
        if(event.type=="key::up") print("Up !{event.key}");
        if(event.type=="key::down") print("Down !{event.key}");
    }
}