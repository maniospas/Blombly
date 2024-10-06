#include "libs/std"
enable std;

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