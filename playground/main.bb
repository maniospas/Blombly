// main.bb

// Below we define a code block (it does not run yet).
// It is made final for visibility from within parallel function calls.
// Blocks can be used as either functions or inlined,
// for example within struct creation.
final Point = {
    str() = { // a nested code block that can be called as a function
        x = this.x;
        y = this.y;
        return "({x}, {y})"; // string formatting
    }
    add(other) => new{ // `=> ..` is `= {return ...}`
        Point: // inline the code block
        // get values from the definition's immediate closure (this..)
        x = this..x+other.x; 
        y = this..y+other.y;
    }
}

a = new {Point:x=1;y=2}
b = new {Point:x=3;y=4}

// we defined `add`and `str`, which overload namesake operations
c = a+b; 
print(!fmt "{a} + {b} = {c}"); 