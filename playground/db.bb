!modify "test_database.db"

db = sqlite("test_database.db");
//db["PRAGMA journal_mode = 'wal';"]; // speedup for adding delete - find more at https://www.sqlite.org/wal.html
db["CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, age INTEGER);"];

n = 100000;
tic = time();
db["BEGIN TRANSACTION;"];
while(i in range(n)) {
    db["INSERT INTO users (name, age) VALUES ('User{i}', {20 + (i % 10)});"];
    db["SELECT * FROM users WHERE id = {i};"];
    db["UPDATE users SET age = age + 1 WHERE id = {i};"];
    //db["DELETE FROM users WHERE id = {i};"];
}
db["COMMIT;"];

toc = time();

print(toc - tic);
while(user in db["SELECT * FROM users;"]) print(user);
db["DELETE FROM users;"];
