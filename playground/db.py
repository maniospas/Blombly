import sqlite3
import time

# Set up the database
conn = sqlite3.connect("test_database.db")
cursor = conn.cursor()
cursor.execute("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, age INTEGER);")
conn.commit()

# Benchmark
tic = time.time()

for i in range(1000):
    cursor.execute("INSERT INTO users (name, age) VALUES (?, ?);", (f"User{i}", 20 + (i % 10)))
    cursor.execute("SELECT * FROM users WHERE id = ?;", (i,))
    cursor.execute("UPDATE users SET age = age + 1 WHERE id = ?;", (i,))
    cursor.execute("DELETE FROM users WHERE id = ?;", (i,))
    conn.commit()

toc = time.time()

print(toc - tic)

# Clean up after benchmark
cursor.execute("DELETE FROM users;")
conn.commit()
conn.close()
