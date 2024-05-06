// Vector.h
#ifndef VECTOR_H
#define VECTOR_H

#include <memory>
#include <string>
#include "Data.h"

// Class representing a raw vector of double values with thread-safe access
class RawVector {
private:
    pthread_mutex_t memoryLock;

public:
    double* data;
    int size;

    explicit RawVector(int siz);
    ~RawVector();

    void lock();
    void unlock();
};

// Vector class representing a multidimensional array of double values
class Vector : public Data {
private:
    int* atdims; // Accessed dimensions for sub-vectors
    int natdims; // Number of accessed dimensions
    int* dims; // Shape of the vector
    int ndims; // Number of dimensions
    std::shared_ptr<RawVector> value; // Raw data

public:
    explicit Vector(int size);
    Vector(int size, bool setToZero);
    Vector(int size1, int size2);
    Vector(int size1, int size2, bool setToZero);
    Vector(std::shared_ptr<RawVector> val, const Vector* prototype);
    Vector(std::shared_ptr<RawVector> val, const Vector* prototype, int new_dim_access);
    Vector(std::shared_ptr<RawVector> val, int size1, int size2);
    ~Vector();

    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<RawVector> getValue() const;
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, const BuiltinArgs& args) override;
};

#endif // VECTOR_H
