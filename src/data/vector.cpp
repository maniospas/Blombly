#include "data/Vector.h"
#include "data/Integer.h"
#include "data/BFloat.h"
#include "data/BString.h"
#include "common.h"
#include "data/Iterator.h"
#include "data/BError.h"
#include <iostream>
#include <cmath>

extern BError* OUT_OF_RANGE;
extern BError* INCOMPATIBLE_SIZES;

#define POS(v, i, j) (i * v + j)

Vector::Vector(uint64_t size) : data(new double[size]), size(size), lockable(0), Data(VECTOR) {
    dims = new int[1];
    dims[0] = size;
    ndims = 1;
    atdims = nullptr;
    natdims = 0;
}

Vector::Vector(uint64_t size, bool setToZero) : Vector(size) {
    if (setToZero) {
        std::fill(data, data + size, 0);
    }
}

Vector::Vector(uint64_t size1, uint64_t size2) : data(new double[size1 * size2]), size(size1 * size2), lockable(0), Data(VECTOR) {
    dims = new int[2];
    dims[0] = size1;
    dims[1] = size2;
    ndims = 2;
    atdims = nullptr;
    natdims = 0;
}

Vector::Vector(uint64_t size1, uint64_t size2, bool setToZero) : Vector(size1, size2) {
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

Vector::Vector(double* data, const Vector* prototype, uint64_t new_dim_access) : data(data), size(prototype->size), lockable(0), Data(VECTOR) {
    ndims = prototype->ndims;
    dims = new int[ndims];
    std::copy(prototype->dims, prototype->dims + ndims, dims);
    atdims = new int[prototype->natdims + 1];
    std::copy(prototype->atdims, prototype->atdims + prototype->natdims, atdims);
    atdims[prototype->natdims] = new_dim_access;
    natdims = prototype->natdims + 1;
}

Vector::Vector(double* data, uint64_t size1, uint64_t size2) : data(data), size(size1 * size2), lockable(0), Data(VECTOR) {
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

std::string Vector::toString(BMemory* memory){
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

Result Vector::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    if (operation == AT && args->size == 2) {
        if (args->arg1->getType() == BB_INT) {
            std::lock_guard<std::recursive_mutex> lock(memoryLock);
            int index = static_cast<Integer*>(args->arg1.get())->getValue();
            if (natdims) {
                for (int i = 0; i < natdims; ++i) {
                    index *= dims[i + 1];
                    index += atdims[i];
                }
            }
            if (index < 0 || index >= size) {
                return Result(OUT_OF_RANGE);
            }
            return std::move(Result(new BFloat(data[index])));
        }

        if (args->arg1->getType() == LIST || args->arg1->getType() == ITERATOR) {
            BuiltinArgs implargs;
            implargs.size = 1;
            implargs.arg0 = args->arg1;

            Result iter = args->arg1->implement(TOITER, &implargs, memory);
            DataPtr iterator = iter.get();
            bbassert(iterator.exists() && iterator->getType() == ITERATOR, "Can only find vector indexes based on an iterable object, but a non-iterable struct was provided.");

            Iterator* iterPtr = static_cast<Iterator*>(iterator.get());

            // Efficiently handle contiguous iterators
            if (iterPtr->isContiguous()) {
                int64_t start = iterPtr->getStart();
                int64_t end = iterPtr->getEnd();
                if (start < 0 || end < 0 || start >= size || end > size) {
                    return std::move(Result(OUT_OF_RANGE));
                }

                auto* resultVec = new Vector(end - start);
                for (int64_t i = start; i < end; ++i) {
                    resultVec->data[i - start] = data[i];
                }
                return std::move(Result(resultVec));
            } else {
                // Handle arbitrary iterators
                auto* resultVec = new Vector(iterPtr->expectedSize());
                int indexCount = 0;

                while (true) {
                    implargs.size = 1;
                    Result next = iterator->implement(NEXT, &implargs, memory);
                    DataPtr indexData = next.get();
                    if (!indexData.exists() || indexData.get() == OUT_OF_RANGE) break;
                    bbassert(indexData->getType() == BB_INT, 
                            "Iterable vector indexes can only contain integers.");

                    int64_t idx = static_cast<Integer*>(indexData.get())->getValue();
                    if (idx < 0 || idx >= size) return std::move(Result(OUT_OF_RANGE));

                    resultVec->data[indexCount++] = data[idx];
                }
                return std::move(Result(resultVec));
            }
        }
    }


    if (operation == PUT && args->size == 3 && args->arg1->getType() == BB_INT) {
        double value;
        if(args->arg2->getType() == BB_FLOAT)
            value = static_cast<BFloat*>(args->arg2.get())->getValue();
        else if(args->arg2->getType() == BB_INT)
            value = static_cast<Integer*>(args->arg2.get())->getValue();
        else
            throw Unimplemented();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        int index = static_cast<Integer*>(args->arg1.get())->getValue();
        for (int i = 0; i < natdims; ++i) {
            index *= dims[i + 1];
            index += atdims[i];
        }
        if (index < 0 || index >= size) {
            //bberror("Vector index " + std::to_string(index) + " out of range [0," + std::to_string(size) + ")");
            return std::move(Result(OUT_OF_RANGE));
        }
        data[index] = value;
        return std::move(Result(INCOMPATIBLE_SIZES));
    }


    if (operation == ADD && args->size == 2 && args->arg0->getType() == VECTOR && args->arg1->getType() == VECTOR) {
        Vector* a1 = static_cast<Vector*>(args->arg0.get());
        Vector* a2 = static_cast<Vector*>(args->arg1.get());

        if (a1->size != a2->size) {
            //bberror("Vector sizes do not match for addition.");
            return std::move(Result(INCOMPATIBLE_SIZES));
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
        Vector* a1 = static_cast<Vector*>(args->arg0.get());
        Vector* a2 = static_cast<Vector*>(args->arg1.get());

        if (a1->size != a2->size) {
            //bberror("Vector sizes do not match for subtraction.");
            return std::move(Result(INCOMPATIBLE_SIZES));
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
        Vector* a1 = static_cast<Vector*>(args->arg0.get());
        Vector* a2 = static_cast<Vector*>(args->arg1.get());
        if (a1->size != a2->size) {
            //bberror("Vector sizes do not match for multiplication.");
            return std::move(Result(INCOMPATIBLE_SIZES));
        }
        std::lock_guard<std::recursive_mutex> lock1(a1->memoryLock);
        std::lock_guard<std::recursive_mutex> lock2(a2->memoryLock);
        auto result = new Vector(a1->size);
        for (int i = 0; i < a1->size; ++i) 
            result->data[i] = a1->data[i] * a2->data[i];
        return std::move(Result(result));
    }

    if (operation == MUL && args->size == 2 && args->arg0->getType() == VECTOR && args->arg1->getType() == VECTOR) {
        Vector* a1 = static_cast<Vector*>(args->arg0.get());
        Vector* a2 = static_cast<Vector*>(args->arg1.get());
        if (a1->size != a2->size) {
            //bberror("Vector sizes do not match for multiplication.");
            return std::move(Result(INCOMPATIBLE_SIZES));
        }
        std::lock_guard<std::recursive_mutex> lock1(a1->memoryLock);
        std::lock_guard<std::recursive_mutex> lock2(a2->memoryLock);
        auto result = new Vector(a1->size);
        for (int i = 0; i < a1->size; ++i) 
            result->data[i] = a1->data[i] / a2->data[i];
        return std::move(Result(result));
    }

    if (operation == POW && args->size == 2 && args->arg0->getType() == VECTOR && args->arg1->getType() == VECTOR) {
        Vector* a1 = static_cast<Vector*>(args->arg0.get());
        Vector* a2 = static_cast<Vector*>(args->arg1.get());

        if (a1->size != a2->size) {
            //bberror("Vector sizes do not match for power operation.");
            return std::move(Result(INCOMPATIBLE_SIZES));
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
        Vector* vec = static_cast<Vector*>(args->arg0.get());

        std::lock_guard<std::recursive_mutex> lock(vec->memoryLock);

        double sum = 0;
        for (int i = 0; i < vec->size; ++i) {
            sum += vec->data[i];
        }
        return std::move(Result(new BFloat(sum)));
    }

    if (operation == MAX && args->size == 1 && args->arg0->getType() == VECTOR) {
        Vector* vec = static_cast<Vector*>(args->arg0.get());

        if (vec->size == 0) {
            //bberror("Cannot apply max on an empty vector.");
            return std::move(Result(OUT_OF_RANGE));
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
        Vector* vec = static_cast<Vector*>(args->arg0.get());

        if (vec->size == 0) {
            //bberror("Cannot apply min on an empty vector.");
            return Result(OUT_OF_RANGE);
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

        Vector* vec = args->arg0->getType() == VECTOR ? static_cast<Vector*>(args->arg0.get()) : static_cast<Vector*>(args->arg1.get());
        double scalar = args->arg0->getType() != VECTOR 
            ? args->arg0->getType()==BB_FLOAT?static_cast<BFloat*>(args->arg0.get())->getValue(): static_cast<Integer*>(args->arg0.get())->getValue()  
            : args->arg1->getType()==BB_FLOAT?static_cast<BFloat*>(args->arg1.get())->getValue(): static_cast<Integer*>(args->arg1.get())->getValue() ;

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
