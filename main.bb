a = new(
    this.x = 0;
    inc = {
        default value=1; 
        this.x = this.x+value;
    }
    sync = {return this;}
);
a.inc(value=2);
print(a.x); // still 0 probably because inc is running in parallel
a = a.sync(); // this pattern forces synchronization
print(a.x); // 2