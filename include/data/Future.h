#ifndef FUTURE_H
#define FUTURE_H

#include <memory>
#include <thread>
#include "data/Data.h"
#include "data/BError.h"
#include "interpreter/Command.h"
#include "interpreter/thread.h"
#include "BMemory.h"
#include "data/Code.h"


class ThreadResult {
public:
    std::thread thread;
    Data* value;
    BBError* error;
    ThreadResult():value(nullptr), error(nullptr) {};
    void start(Code* code, BMemory* newMemory, ThreadResult* futureResult, Command *command) {
        thread = std::thread(threadExecute, code, newMemory, futureResult, command);
    }
};


class Future : public Data {
private:
    static int max_threads;
    static std::atomic<int> thread_count;
    mutable std::recursive_mutex syncMutex;
    ThreadResult* result;

public:
    Future();
    explicit Future(ThreadResult* result);
    ~Future();

    std::string toString() const override;
    Data* getResult() const;

    static bool acceptsThread();
    static void setMaxThreads(int maxThreads);
};

#endif // FUTURE_H
