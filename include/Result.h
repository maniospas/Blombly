#ifndef RESULT_H
#define RESULT_H

#include "common.h"

class Result {
private:
    DataPtr data;

public:
    explicit Result(DataPtr data) noexcept;
    //explicit Result(const DataPtr& data) noexcept;
    explicit Result(Result& other) noexcept;
    Result(Result&& other) noexcept;
    ~Result();
    Result& operator=(const Result& other);
    Result& operator=(Result&& other) noexcept;
    const DataPtr& get() const;
};

#endif // RESULT_H
