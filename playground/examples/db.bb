db = bb.db(":memory:");
users = db.table("users", "id INTEGER PRIMARY KEY, name TEXT, age INTEGER");

tic = time();
while(i in range(10000)) do {
    do end = db.transaction();
    defer end();

    user = map();
    user["name"] = "'User{i}'";
    user["age"] = i+20;
    users.insert(user);
}
print(time()-tic);

print(users.select("*")|len);
