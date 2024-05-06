// Future.h
#ifndef FUTURE_H
#define FUTURE_H

#include <memory>
#include <thread>
#include "Data.h"

// ThreadResult class for holding thread execution results
class ThreadResult {
public:
    std::shared_ptr<class Data> value;
    ThreadResult() = default;
};

// FutureData class holding thread and result information
class FutureData {
public:
    std::thread thread;
    std::shared_ptr<ThreadResult> result;

    FutureData();
    ~FutureData();
};

// Future class representing an asynchronous computation
class Future : public Data {
private:
    std::shared_ptr<FutureData> data;

public:
    explicit Future(std::shared_ptr<FutureData> data);

    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> getResult();
};

#endif // FUTURE_H
