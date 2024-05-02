# Execution modes

**Inline:** The code block runs sequentially and immediately within the current scope. Also used by all conditions and loops. 

**New:** The code block runs sequentially in a new scope that inheritst the previous one but also keeps new assignments. Creates a *this* variable to be returned. Also used when passing arguments to calls. Any returns change the outcome of new but do not affect the parent block's execution. 

**Call:** The code block runs in a thread in parallel. It first runs the arguments and passes those. The running block can see only the final variables from the scope where it is called. Waiting for the thread to
conclude is only done if its result is used.