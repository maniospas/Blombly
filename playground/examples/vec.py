import numpy as np
import time
import gc

gc.collect()

# Create two large vectors with 1e9 elements each
size = 100000000
vector_a = np.zeros(size, dtype=np.float64)
vector_b = np.zeros(size, dtype=np.float64)

# Measure the time for adding two vectors
start_time = time.time()
result = vector_a + vector_b
end_time = time.time()

# Calculate elapsed time
elapsed_time = end_time - start_time
print(elapsed_time)
