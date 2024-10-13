#ifndef RESULT_H
#define RESULT_H

class Data;

class Result {
private:
    Data* data;

public:
    explicit Result(Data* data);
    explicit Result(const Result& other);
    explicit Result(Result& other);
    explicit Result(Result&& other) noexcept;
    ~Result();
    Result& operator=(const Result& other);
    Result& operator=(Result&& other) noexcept;
    Data* get() const;
};

#endif // RESULT_H
