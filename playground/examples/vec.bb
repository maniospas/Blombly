
size = 10000000;
vector_a = vector(size);
vector_b = vector(size);

start_time = time();
result = vector_a + vector_b;
end_time = time();

print(result|sum);
elapsed_time = end_time - start_time;
print(elapsed_time)
