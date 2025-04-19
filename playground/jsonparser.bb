a = bb.os.read("playground/jsontest.json"); 
obj = bb.json.parse(a);
print(obj);

bb:
obj = json.list(
    json.map(("A", 2), ("B",3)),
    json.NULL
);
print(obj);
print(json.parse(obj));