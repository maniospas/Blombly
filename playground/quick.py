import time
import random

class Value:
    def __init__(self, value):
        self.value = value

    def __repr__(self):
        return f"Value({self.value})"

def quicksort_inplace(arr, low, high, key):
    if low < high:
        pivot_index = partition(arr, low, high, key)
        quicksort_inplace(arr, low, pivot_index - 1, key)
        quicksort_inplace(arr, pivot_index + 1, high, key)

def partition(arr, low, high, key):
    pivot = key(arr[high])
    i = low - 1
    for j in range(low, high):
        if key(arr[j]) <= pivot:
            i += 1
            arr[i], arr[j] = arr[j], arr[i]
    arr[i + 1], arr[high] = arr[high], arr[i + 1]
    return i + 1

# Generate an array of 300 Value instances with random values
data = [Value(random.randint(1, 1000)) for _ in range(300)]

# Function to extract the value field
def get_value(obj):
    return obj.value

# Measure sorting time
start_time = time.time()
quicksort_inplace(data, 0, len(data) - 1, get_value)
end_time = time.time()

# Display results
print("Sorted Data:", data)
print(f"Sorting took {end_time - start_time:.6f} seconds")
