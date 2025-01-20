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
    Result value;
    BBError* error;
    ThreadResult():value(nullptr), error(nullptr) {};
    ~ThreadResult() = default;
    void start(Code* code, BMemory* newMemory, ThreadResult* futureResult, const Command* command, DataPtr thisObj) {thread = std::thread(threadExecute, code, newMemory, futureResult, command, thisObj);}
};


class Future : public Data {
private:
    static int max_threads;
    static std::atomic<int> thread_count;
    ThreadResult* result;

public:
    Future();
    explicit Future(ThreadResult* result);
    ~Future();

    std::string toString(BMemory* memory)override;
    Result getResult() const;

    static bool acceptsThread();
    static void setMaxThreads(int maxThreads);
};

#endif // FUTURE_H
