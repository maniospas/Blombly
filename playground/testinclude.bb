create_adder = {
    default inc = 0;
    return new{call(a,b)=>a+b+this..inc}
}