db = sqlite(":memory:");
db << "PRAGMA journal_mode = 'wal';"; // often speeds things up (https://www.sqlite.org/wal.html)
db << "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, age INTEGER);";

while(i in range(5)) {
    db << "BEGIN TRANSACTION;"; // instead of this, prefer batching operations into larger transactions
        db << "INSERT INTO users (name, age) VALUES ('User{i}', {20 + (i % 10)});";
        db << "SELECT * FROM users WHERE id = {i};";
        db << "UPDATE users SET age = age + 1 WHERE id = {i};";
        //db["DELETE FROM users WHERE id = {i};"]; // deletes are generally slow
    db << "COMMIT;";
}

while(user in db<<"SELECT * FROM users;") print(user);
db << "DELETE FROM users;";