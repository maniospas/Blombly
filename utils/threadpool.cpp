#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <stack>
#include <map>
#include <sstream>
#include <memory> 
#include <cstdlib>
#include <algorithm>
#include <unordered_set>  
#include <pthread.h>
#include <queue>
#include <atomic>
#include <functional>
#include <thread>
#include <cmath>


class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false), nextTaskId(0) {
        for (size_t i = 0; i < numThreads; ++i) {
            pthread_t tid;
            pthread_create(&tid, nullptr, workerThread, this);
            threads.push_back(tid);
        }
    }

    // Function to add tasks to the thread pool and return a unique task identifier
    int enqueue(std::function<void*(void*)> function, void* data) {
        pthread_mutex_lock(&queueMutex);
        int taskId = nextTaskId++;
        tasks.push({ taskId, function, data });
        pthread_mutex_unlock(&queueMutex);
        pthread_cond_signal(&taskCondition);
        return taskId;
    }

    // Function to wait for a specific task to complete based on its task identifier
    void join(int taskId, void** returnValue) {
        pthread_mutex_lock(&joinMutex);
        while (!completedTasks[taskId]) {
            pthread_cond_wait(&joinCondition, &joinMutex);
        }
        *returnValue = taskOutputs[taskId];
        pthread_mutex_unlock(&joinMutex);
    }

    ~ThreadPool() {
        stop = true;
        pthread_cond_broadcast(&taskCondition);
        for (pthread_t tid : threads) {
            pthread_join(tid, nullptr);
        }
    }

private:
    struct Task {
        int taskId;
        std::function<void*(void*)> function;
        void* data;
    };

    static void* workerThread(void* arg) {
        ThreadPool* pool = static_cast<ThreadPool*>(arg);
        while (true) {
            pthread_mutex_lock(&pool->queueMutex);
            while (pool->tasks.empty() && !pool->stop) {
                pthread_cond_wait(&pool->taskCondition, &pool->queueMutex);
            }

            if (pool->stop && pool->tasks.empty()) {
                pthread_mutex_unlock(&pool->queueMutex);
                break;
            }

            Task task = pool->tasks.front();
            pool->tasks.pop();
            pthread_mutex_unlock(&pool->queueMutex);

            void* ret = task.function(task.data);

            pthread_mutex_lock(&pool->joinMutex);
            pool->completedTasks[task.taskId] = true;
            pool->taskOutputs[task.taskId] = ret;
            pthread_cond_signal(&pool->joinCondition);
            pthread_mutex_unlock(&pool->joinMutex);
        }
        pthread_exit(nullptr);
    }

    std::vector<pthread_t> threads;
    std::queue<Task> tasks;
    std::unordered_map<int, bool> completedTasks;
    std::unordered_map<int, void*> taskOutputs;
    pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t joinMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t taskCondition = PTHREAD_COND_INITIALIZER;
    pthread_cond_t joinCondition = PTHREAD_COND_INITIALIZER;
    bool stop;
    std::atomic<int> nextTaskId;
};