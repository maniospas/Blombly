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
    std::shared_ptr<double[]> data;  // Raw data
    
    explicit Vector(int size);
    Vector(int size, bool setToZero);
    Vector(int size1, int size2);
    Vector(int size1, int size2, bool setToZero);
    Vector(const std::shared_ptr<double[]>& data, const Vector* prototype);
    Vector(const std::shared_ptr<double[]>& data, const Vector* prototype, int new_dim_access);
    Vector(const std::shared_ptr<double[]>& data, int size1, int size2);
    ~Vector();

    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<double[]> getValue() const;
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;

    void lock() const;
    void unlock() const;

    friend class BFloat;
    friend class BList;
};

#endif // VECTOR_H
