command = read("What do you want to do?");
a = try {  // or `result = try {...}` if you are sure you are going to return a value
   if(command=="nothing")
       return;
   print("Instructions unclear. let's add two numbers.");
   a = read("First");
   b = read("Second");
   c = float(a) + float(b);
   print(c);
}