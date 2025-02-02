final Todo = {
    id = 0;
    title = "";
    completed = false;
}
final null = {}
final todo_list = new {
    todos = list();
    next_id = 1;
    add_todo(title) = {
        todo = new {
            Todo:
            id = this.next_id;
            title = title;
            completed = false;
        }
        this.todos << todo;
        this.next_id += 1;
        return todo;
    }
    get_todos() => this.todos;
    get_todo(id) = {
        while(todo in this.todos) if (todo.id == id) return todo;
        return null;
    }

    // Update a to-do's completion status
    update_todo(id, completed) = {
        todo = this.get_todo(id);
        if (todo != null) {
            todo.completed = completed;
            return todo;
        }
    }

    // Delete a to-do by ID
    delete_todo(id) = {
        index = -1;
        do while (i in range(this.todos|len)) if (this.todos[i].id == id) {
            index = i;
            return null;
        }
        if (index != -1) {
            deleted_todo = this.todos[index];
            this.todos = this.todos[range(index)] + this.todos[range(index+1, this.todos|len)];
            return deleted_todo;
        }
        return null;
    }
}

routes = server(8000);
routes["/"] => new {
    type = "text/html";
    str => "
    <!DOCTYPE html>
    <html>
    <head>
        <title>Blombly To-Do List</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 20px; }
            h1 { color: #333; }
            #todo-list { list-style-type: none; padding: 0; }
            .todo-item { display: flex; align-items: center; margin-bottom: 10px; }
            .todo-item input[type='checkbox'] { margin-right: 10px; }
            .todo-item.completed { text-decoration: line-through; color: #888; }
            #new-todo { margin-top: 20px; }
            #new-todo input { padding: 5px; width: 200px; }
            #new-todo button { padding: 5px 10px; }
        </style>
    </head>
    <body>
        <h1>Blombly To-Do List</h1>
        <ul id='todo-list'></ul>
        <div id='new-todo'>
            <input type='text' id='todo-input' placeholder='Add a new to-do'>
            <button onclick='addTodo()'>Add</button>
        </div>
        <script type='text/javascript'>
            // Fetch and display to-dos
            function fetchTodos() {
                fetch('/todos')
                    .then(response => response.json())
                    .then(todos => {
                        const todoList = document.getElementById('todo-list');
                        todoList.innerHTML = '';
                        todos.forEach(todo => {
                            const li = document.createElement('li');
                            li.className = 'todo-item' + (todo.completed ? ' completed' : '');
                            li.innerHTML = `
                                <input type='checkbox' ${todo.completed ? 'checked' : ''} onchange='toggleTodo(${todo.id})'>
                                ${todo.title}
                                <button onclick='deleteTodo(${todo.id})'>Delete</button>
                            `;
                            todoList.appendChild(li);
                        });
                    });
            }

            // Add a new to-do
            function addTodo() {
                const input = document.getElementById('todo-input');
                const title = input.value.trim();
                if (title) {
                    fetch('/todos', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ title })
                    })
                    .then(() => {
                        input.value = '';
                        fetchTodos();
                    });
                }
            }

            // Toggle to-do completion status
            function toggleTodo(id) {
                fetch(`/todos/${id}`, {
                    method: 'PUT',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ completed: event.target.checked })
                })
                .then(fetchTodos);
            }

            // Delete a to-do
            function deleteTodo(id) {
                fetch(`/todos/${id}`, { method: 'DELETE' })
                    .then(fetchTodos);
            }
            console.log('here');

            // Initial fetch
            fetchTodos();
        </script>
    </body>
    </html>
    ";
}

server::method = "";
print(server::method);
server::query = "";
print(server::query);


// Define REST API routes
routes["/todos"] = {
    print(server::method);
    if(server::method == "GET") return todo_list.get_todos();
    if(server::method == "POST") {
        title = server::query|str;
        return todo_list.add_todo(title);
    }
}

routes["/todos/<id>"] = {
    id = int(id);
    if(server::method == "GET") return todo_list.get_todo(id);
    if(server::method == "PUT") {
        completed = server::query|str;
        return todo_list.update_todo(id, completed);
    }
    if(server::method == "DELETE") return todo_list.delete_todo(id);
}

// Wait indefinitely
while (true) {}