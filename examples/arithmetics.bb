// this is a test suit for the blombly language's arithmetics
result = {
    if(1!=1, return false);
    if(1==2, return false);
    if(1+4/2!=3, return false);
    if((2+4)/2!=3, return false);
    if(1>2, return false);
    if(1>=2, return false);
    if(1<=0, return false);
    if(1<0, return false);
    if(1==1 && 1<1, return false);
    if(1<1 && 1==1, return false);
    return "all tests passed";
}

print(result()!="failed");