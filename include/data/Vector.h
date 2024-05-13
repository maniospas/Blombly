// data/Vector.h
#ifndef VECTOR_H
#define VECTOR_H

#include <memory>
#include <string>
#include "data/Data.h"

// Class representing a raw vector of double values with thread-safe access
class RawVector {
private:
    pthread_mutex_t memoryLock;

public:
    double* data;
    int size;
    int lockable; // counts shared instances-1

    explicit RawVector(int siz);
    ~RawVector();

    void lock();
    void unlock();
    void unsafeUnlock(); // used only in Vector's destructor
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
    Vector(const std::shared_ptr<RawVector>& val, const Vector* prototype);
    Vector(const std::shared_ptr<RawVector>& val, const Vector* prototype, int new_dim_access);
    Vector(const std::shared_ptr<RawVector>& val, int size1, int size2);
    ~Vector();

    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<RawVector> getValue() const;
    Data* shallowCopy() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;

    friend class BFloat;
};

#endif // VECTOR_H
