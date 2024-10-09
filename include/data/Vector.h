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
    int size;         // Size of the vector
    mutable std::recursive_mutex memoryLock;   // Mutex for thread safety
    int lockable;     // Counts shared instances-1

public:
    double* data;  // Raw data
    
    explicit Vector(int size);
    Vector(int size, bool setToZero);
    Vector(int size1, int size2);
    Vector(int size1, int size2, bool setToZero);
    Vector(double* data, const Vector* prototype);
    Vector(double* data, const Vector* prototype, int new_dim_access);
    Vector(double* data, int size1, int size2);
    ~Vector();

    std::string toString() const override;
    double* getValue() const;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;

    void lock() const;
    void unlock() const;

    friend class BFloat;
    friend class BList;
};

#endif // VECTOR_H
