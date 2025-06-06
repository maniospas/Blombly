!modify "bb://.cache/database.db"

db = sqlite("bb://.cache/database.db");
db << "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, age INTEGER);";

n = 10000;
start = time();

db << "BEGIN TRANSACTION;";
while(i in range(n)) {
    db << "INSERT INTO users (name, age) VALUES ('User!{i}', !{20 + (i % 10)});";
    db << "SELECT * FROM users WHERE id = !{i};";
    db << "UPDATE users SET age = age + 1 WHERE id = !{i};";
}
db << "COMMIT;";

eta = time()-start;
assert eta < 1;
assert n == (db << "SELECT * FROM users;")|len;
db << "DELETE FROM users;";
