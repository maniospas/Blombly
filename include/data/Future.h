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
    std::shared_ptr<Data> value;
    std::shared_ptr<BBError> error;
    ThreadResult():value(nullptr), error(nullptr) {};
    void start(const std::shared_ptr<Code>& code, const std::shared_ptr<BMemory>& newMemory, const std::shared_ptr<ThreadResult>& futureResult, Command *command) {
        thread = std::thread(threadExecute, std::move(std::static_pointer_cast<Code>(INLINE_SCOPY(code))), std::move(newMemory), std::move(futureResult), command);
    }
};


class Future : public Data {
private:
    static int max_threads;
    static int thread_count;
    std::shared_ptr<ThreadResult> result;
    mutable std::recursive_mutex syncMutex;

public:
    Future();
    explicit Future(std::shared_ptr<ThreadResult> result);
    ~Future();

    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> getResult() const;

    static bool acceptsThread();
    static void setMaxThreads(int maxThreads);
};

#endif // FUTURE_H
