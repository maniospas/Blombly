Value = {
    str() => "v"+str(this.value);
}

final quicksort_inplace(arr, low, high) = {
    if (low < high) {
        pivot_index = partition(arr, low, high);
        quicksort_inplace(arr, low, pivot_index - 1);
        quicksort_inplace(arr, pivot_index + 1, high);
    }
}

final partition(arr, low, high) = {
    pivot = key(arr[high]);
    i = low - 1;
    while(j in range(low, high)){
        if (key(arr[j]) <= pivot) {
            i += 1;
            tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }
    }
    i += 1;
    tmp = arr[i];
    arr[i] = arr[high];
    arr[high] = tmp;
    return i;
}

rand = random(42);
data = !gather(list(), <<){while(i in range(300)) yield new{Value:value=rand|next;}}

// Function to extract the value field
final key(obj) => obj.value;

// Measure sorting time
start_time = time();
quicksort_inplace(data, 0, len(data) - 1);
print("Sorted Data len: !{data|len}");
end_time = time();

print(data);

// Display results
print("Sorting took !{end_time - start_time} seconds");
