#ifndef VECTOR_H
#define VECTOR_H

#include <memory>
#include <string>
#include <mutex>
#include "data/Data.h"


class Vector : public Data {
private:
    int* atdims;      // Accessed dimensions for sub-vectors
    int natdims;      // Number of accessed dimensions
    int* dims;        // Shape of the vector
    int ndims;        // Number of dimensions
    uint64_t size;         // Size of the vector
    mutable std::recursive_mutex memoryLock;   // Mutex for thread safety
    int lockable;     // Counts shared instances-1

public:
    double* data;  // Raw data
    
    explicit Vector(uint64_t size);
    Vector(uint64_t size, bool setToZero);
    Vector(uint64_t size1, uint64_t size2);
    Vector(uint64_t size1, uint64_t size2, bool setToZero);
    Vector(double* data, const Vector* prototype);
    Vector(double* data, const Vector* prototype, uint64_t new_dim_access);
    Vector(double* data, uint64_t size1, uint64_t size2);
    ~Vector();

    std::string toString()override;
    double* getValue() const;
    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;

    void lock() const;
    void unlock() const;

    friend class BFloat;
    friend class BList;
};

#endif // VECTOR_H
