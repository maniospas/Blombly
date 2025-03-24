// Function to extract the value field
//final key(obj) => obj.value;
!local{key(@obj)} as {@obj.value}

Value = {
    str() => "v"+str(this.value);
}

final quickstort(list arr) = {
    final inplace = {
        final low = low;
        final high = high;
        final arr = arr;
        if(low < high) {
            pivot_index = partition();
            inplace(high=pivot_index - 1);
            inplace(low=pivot_index + 1);
        }
    }
    final partition = {
        pivot = key(arr[high]);
        i = low - 1;
        while(j in range(low, high)) if (key(arr[j]) <= pivot) {
            i += 1;
            tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }
        i += 1;
        tmp = arr[i];
        arr[i] = arr[high];
        arr[high] = tmp;
        return i;
    }
    inplace(arr=arr;low=0;high=arr|len-1);
}

rand = random(42);
data = !gather(list(), <<){while(i in range(300)) yield new{Value:value=rand|next;}}

// Measure sorting time
start_time = time();
quickstort(data);
print("Sorted Data len: !{data|len}");
end_time = time();

print(data);

// Display results
print("Sorting took !{end_time - start_time} seconds");
