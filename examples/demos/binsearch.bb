final search = {
    default start = 0;
    default end = len(list)-1;
    if(start>end, return new(found=false));
    middle = int((start + end)/2);
    if(list[middle]==query, return new(found=true, pos=middle));
    if(list[middle]<query, start=middle+1, end=middle-1);
    search:  // inline recursion
}

z = List(1,2,3,5,6,7);
res = search(list=z, query=7);
if(res.found, 
    print("Query found at index", res.pos), 
    print("Query not found")
);