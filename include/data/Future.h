// data/Future.h
#ifndef FUTURE_H
#define FUTURE_H

#include <memory>
#include <thread>
#include "data/Data.h"

// ThreadResult class for holding thread execution results
class ThreadResult {
public:
    Data* value;
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
    explicit Future(const std::shared_ptr<FutureData>& data);

    int getType() const override;
    std::string toString() const override;
    Data* shallowCopy() const override;
    Data* getResult() const;
};

#endif // FUTURE_H
