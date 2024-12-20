#include "data/Vector.h"
#include "data/Integer.h"
#include "data/BFloat.h"
#include "data/BString.h"
#include "common.h"
#include "data/Iterator.h"
#include <iostream>
#include <cmath>

#define POS(v, i, j) (i * v + j)

Vector::Vector(int size) : data(new double[size]), size(size), lockable(0), Data(VECTOR) {
    dims = new int[1];
    dims[0] = size;
    ndims = 1;
    atdims = nullptr;
    natdims = 0;
}

Vector::Vector(int size, bool setToZero) : Vector(size) {
    if (setToZero) {
        std::fill(data, data + size, 0);
    }
}

Vector::Vector(int size1, int size2) : data(new double[size1 * size2]), size(size1 * size2), lockable(0), Data(VECTOR) {
    dims = new int[2];
    dims[0] = size1;
    dims[1] = size2;
    ndims = 2;
    atdims = nullptr;
    natdims = 0;
}

Vector::Vector(int size1, int size2, bool setToZero) : Vector(size1, size2) {
    if (setToZero) {
        std::fill(data, data + size, 0);
    }
}

Vector::Vector(double* data, const Vector* prototype) : data(data), size(prototype->size), lockable(0), Data(VECTOR) {
    ndims = prototype->ndims;
    dims = new int[ndims];
    std::copy(prototype->dims, prototype->dims + ndims, dims);
    atdims = prototype->atdims;
    natdims = prototype->natdims;
}

Vector::Vector(double* data, const Vector* prototype, int new_dim_access) : data(data), size(prototype->size), lockable(0), Data(VECTOR) {
    ndims = prototype->ndims;
    dims = new int[ndims];
    std::copy(prototype->dims, prototype->dims + ndims, dims);
    atdims = new int[prototype->natdims + 1];
    std::copy(prototype->atdims, prototype->atdims + prototype->natdims, atdims);
    atdims[prototype->natdims] = new_dim_access;
    natdims = prototype->natdims + 1;
}

Vector::Vector(double* data, int size1, int size2) : data(data), size(size1 * size2), lockable(0), Data(VECTOR) {
    dims = new int[2];
    dims[0] = size1;
    dims[1] = size2;
    ndims = 2;
    atdims = nullptr;
    natdims = 0;
}

Vector::~Vector() {
    delete[] dims;
}

std::string Vector::toString() const {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    std::string result = "[";
    for (std::size_t i = 0; i < std::min(static_cast<std::size_t>(size), static_cast<std::size_t>(10)); ++i) {
        if (result.size() > 1) {
            result += ", ";
        }
        result += std::to_string(data[i]);
    }
    if (size > 10) {
        result += ", ...";
    }
    return result + "]";
}

double* Vector::getValue() const {
    return data;
}

void Vector::lock() const {
    memoryLock.lock();
}

void Vector::unlock() const {
    memoryLock.unlock();
}

Result Vector::implement(const OperationType operation, BuiltinArgs* args) {
    if (operation == AT && args->size == 2 && args->arg1->getType() == BB_INT) {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        int index = static_cast<Integer*>(args->arg1)->getValue();
        if (natdims) {
            for (int i = 0; i < natdims; ++i) {
                index *= dims[i + 1];
                index += atdims[i];
            }
        }
        if (index < 0 || index >= size) {
            //bberror("Vector index " + std::to_string(index) + " out of range [0," + std::to_string(size) + ")");
            return Result(nullptr);
        }
        return std::move(Result(new BFloat(data[index])));
    }

    if (operation == PUT && args->size == 3 && args->arg1->getType() == BB_INT) {
        double value;
        if(args->arg2->getType() == BB_FLOAT)
            value = static_cast<BFloat*>(args->arg2)->getValue();
        else if(args->arg2->getType() == BB_INT)
            value = static_cast<Integer*>(args->arg2)->getValue();
        else
            throw Unimplemented();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        int index = static_cast<Integer*>(args->arg1)->getValue();
        for (int i = 0; i < natdims; ++i) {
            index *= dims[i + 1];
            index += atdims[i];
        }
        if (index < 0 || index >= size) {
            bberror("Vector index " + std::to_string(index) + " out of range [0," + std::to_string(size) + ")");
            return std::move(Result(nullptr));
        }
        data[index] = value;
        return std::move(Result(nullptr));
    }


    if (operation == ADD && args->size == 2 && args->arg0->getType() == VECTOR && args->arg1->getType() == VECTOR) {
        Vector* a1 = static_cast<Vector*>(args->arg0);
        Vector* a2 = static_cast<Vector*>(args->arg1);

        if (a1->size != a2->size) {
            bberror("Vector sizes do not match for addition.");
            return std::move(Result(nullptr));
        }

        std::lock_guard<std::recursive_mutex> lock1(a1->memoryLock);
        std::lock_guard<std::recursive_mutex> lock2(a2->memoryLock);

        auto result = new Vector(a1->size);
        for (int i = 0; i < a1->size; ++i) {
            result->data[i] = a1->data[i] + a2->data[i];
        }
        return std::move(Result(result));
    }

    if (operation == SUB && args->size == 2 && args->arg0->getType() == VECTOR && args->arg1->getType() == VECTOR) {
        Vector* a1 = static_cast<Vector*>(args->arg0);
        Vector* a2 = static_cast<Vector*>(args->arg1);

        if (a1->size != a2->size) {
            bberror("Vector sizes do not match for subtraction.");
            return std::move(Result(nullptr));
        }

        std::lock_guard<std::recursive_mutex> lock1(a1->memoryLock);
        std::lock_guard<std::recursive_mutex> lock2(a2->memoryLock);

        auto result = new Vector(a1->size);
        for (int i = 0; i < a1->size; ++i) {
            result->data[i] = a1->data[i] - a2->data[i];
        }
        return std::move(Result(result));
    }

    if (operation == MUL && args->size == 2 && args->arg0->getType() == VECTOR && args->arg1->getType() == VECTOR) {
        Vector* a1 = static_cast<Vector*>(args->arg0);
        Vector* a2 = static_cast<Vector*>(args->arg1);
        if (a1->size != a2->size) {
            bberror("Vector sizes do not match for multiplication.");
            return std::move(Result(nullptr));
        }
        std::lock_guard<std::recursive_mutex> lock1(a1->memoryLock);
        std::lock_guard<std::recursive_mutex> lock2(a2->memoryLock);
        auto result = new Vector(a1->size);
        for (int i = 0; i < a1->size; ++i) 
            result->data[i] = a1->data[i] * a2->data[i];
        return std::move(Result(result));
    }

    if (operation == MUL && args->size == 2 && args->arg0->getType() == VECTOR && args->arg1->getType() == VECTOR) {
        Vector* a1 = static_cast<Vector*>(args->arg0);
        Vector* a2 = static_cast<Vector*>(args->arg1);
        if (a1->size != a2->size) {
            bberror("Vector sizes do not match for multiplication.");
            return std::move(Result(nullptr));
        }
        std::lock_guard<std::recursive_mutex> lock1(a1->memoryLock);
        std::lock_guard<std::recursive_mutex> lock2(a2->memoryLock);
        auto result = new Vector(a1->size);
        for (int i = 0; i < a1->size; ++i) 
            result->data[i] = a1->data[i] / a2->data[i];
        return std::move(Result(result));
    }

    if (operation == POW && args->size == 2 && args->arg0->getType() == VECTOR && args->arg1->getType() == VECTOR) {
        Vector* a1 = static_cast<Vector*>(args->arg0);
        Vector* a2 = static_cast<Vector*>(args->arg1);

        if (a1->size != a2->size) {
            bberror("Vector sizes do not match for power operation.");
            return std::move(Result(nullptr));
        }

        std::lock_guard<std::recursive_mutex> lock1(a1->memoryLock);
        std::lock_guard<std::recursive_mutex> lock2(a2->memoryLock);

        auto result = new Vector(a1->size);
        for (int i = 0; i < a1->size; ++i) {
            result->data[i] = std::pow(a1->data[i], a2->data[i]);
        }
        return std::move(Result(result));
    }

    if (operation == SUM && args->size == 1 && args->arg0->getType() == VECTOR) {
        Vector* vec = static_cast<Vector*>(args->arg0);

        std::lock_guard<std::recursive_mutex> lock(vec->memoryLock);

        double sum = 0;
        for (int i = 0; i < vec->size; ++i) {
            sum += vec->data[i];
        }
        return std::move(Result(new BFloat(sum)));
    }

    if (operation == MAX && args->size == 1 && args->arg0->getType() == VECTOR) {
        Vector* vec = static_cast<Vector*>(args->arg0);

        if (vec->size == 0) {
            bberror("Cannot apply max on an empty vector.");
            return std::move(Result(nullptr));
        }

        std::lock_guard<std::recursive_mutex> lock(vec->memoryLock);

        double maxVal = vec->data[0];
        for (int i = 1; i < vec->size; ++i) {
            if (vec->data[i] > maxVal) {
                maxVal = vec->data[i];
            }
        }
        return std::move(Result(new BFloat(maxVal)));
    }


    if (operation == MIN && args->size == 1 && args->arg0->getType() == VECTOR) {
        Vector* vec = static_cast<Vector*>(args->arg0);

        if (vec->size == 0) {
            bberror("Cannot apply min on an empty vector.");
            return Result(nullptr);
        }

        std::lock_guard<std::recursive_mutex> lock(vec->memoryLock);

        double minVal = vec->data[0];
        for (int i = 1; i < vec->size; ++i) {
            if (vec->data[i] < minVal) {
                minVal = vec->data[i];
            }
        }
        return std::move(Result(new BFloat(minVal)));
    }


    if ((operation == ADD || operation == SUB || operation == MUL || operation == DIV || operation == POW) && 
        args->size == 2 && 
        (args->arg0->getType() == BB_FLOAT || args->arg0->getType() == BB_INT || args->arg1->getType() == BB_FLOAT || args->arg1->getType() == BB_INT)) {

        Vector* vec = args->arg0->getType() == VECTOR ? static_cast<Vector*>(args->arg0) : static_cast<Vector*>(args->arg1);
        double scalar = args->arg0->getType() != VECTOR 
            ? args->arg0->getType()==BB_FLOAT?static_cast<BFloat*>(args->arg0)->getValue(): static_cast<Integer*>(args->arg0)->getValue()  
            : args->arg1->getType()==BB_FLOAT?static_cast<BFloat*>(args->arg1)->getValue(): static_cast<Integer*>(args->arg1)->getValue() ;

        std::lock_guard<std::recursive_mutex> lock(vec->memoryLock);

        auto result = new Vector(vec->size);
        if (operation == ADD) {
            for (int i = 0; i < vec->size; ++i) {
                result->data[i] = vec->data[i] + scalar;
            }
        } else if (operation == SUB) {
            if (args->arg0->getType() == VECTOR) {
                for (int i = 0; i < vec->size; ++i) {
                    result->data[i] = vec->data[i] - scalar;
                }
            } else {
                for (int i = 0; i < vec->size; ++i) {
                    result->data[i] = scalar - vec->data[i];
                }
            }
        } else if (operation == MUL) {
            for (int i = 0; i < vec->size; ++i) {
                result->data[i] = vec->data[i] * scalar;
            }
        } else if (operation == DIV) {
            if (args->arg0->getType() == VECTOR) {
                for (int i = 0; i < vec->size; ++i) {
                    result->data[i] = vec->data[i] / scalar;
                }
            } else {
                for (int i = 0; i < vec->size; ++i) {
                    result->data[i] = scalar / vec->data[i];
                }
            }
        } else if (operation == POW) {
            if (args->arg0->getType() == VECTOR) {
                for (int i = 0; i < vec->size; ++i) {
                    result->data[i] = std::pow(vec->data[i], scalar);
                }
            } else {
                for (int i = 0; i < vec->size; ++i) {
                    result->data[i] = std::pow(scalar, vec->data[i]);
                }
            }
        }

        return std::move(Result(result));
    }

    
    if(operation==TOITER && args->size==1) 
        return std::move(Result(new AccessIterator(args->arg0)));
    
    if(operation==TOVECTOR && args->size==1) 
        return std::move(Result(this));

    if(operation==LEN && args->size==1) 
        return std::move(Result(new Integer(dims[0])));

    throw Unimplemented();
}
