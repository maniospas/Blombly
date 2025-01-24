import numpy as np
import time

# Create two large vectors with 1e9 elements each
size = int(1e8)
vector_a = np.zeros(size)
vector_b = np.zeros(size)

# Measure the time for adding two vectors
start_time = time.time()
result = vector_a + vector_b
end_time = time.time()

# Calculate elapsed time
elapsed_time = end_time - start_time
print(elapsed_time)
