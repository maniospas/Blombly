// Vector.cpp
#include "Vector.h"
#include "Integer.h"
#include "bFloat.h"
#include "BString.h"
#include "common.h"
#include "Iterator.h"
#include <iostream>
#include <cmath>
# define POS(v, i, j) (i*v+j)


// RawVector constructor and destructor
RawVector::RawVector(int siz) {
    size = siz;
    data = new double[size];
    lockable = 0;
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) {
        std::cerr << "Failed to create a mutex for vector read/write" << std::endl;
        exit(1);
    }
}

RawVector::~RawVector() {
    delete[] data;
}

void RawVector::lock() {
    if(lockable)
        pthread_mutex_lock(&memoryLock);
}

void RawVector::unlock() {
    if(lockable)
        pthread_mutex_unlock(&memoryLock);
}

void RawVector::unsafeUnlock() {
    pthread_mutex_unlock(&memoryLock);
}

// Vector constructors and destructor
Vector::Vector(int size) : value(std::make_shared<RawVector>(size)) {
    dims = new int[1];
    dims[0] = size;
    ndims = 1;
    atdims = nullptr;
    natdims = 0;
}

Vector::Vector(int size, bool setToZero) : Vector(size) {
    if (setToZero) {
        std::fill(value->data, value->data + value->size, 0);
    }
}

Vector::Vector(int size1, int size2) : value(std::make_shared<RawVector>(size1 * size2)) {
    dims = new int[2];
    dims[0] = size1;
    dims[1] = size2;
    ndims = 2;
    atdims = nullptr;
    natdims = 0;
}

Vector::Vector(int size1, int size2, bool setToZero) : Vector(size1, size2) {
    if (setToZero) {
        std::fill(value->data, value->data + value->size, 0);
    }
}

Vector::Vector(const std::shared_ptr<RawVector>& val, const Vector* prototype) {
    value = val;
    ndims = prototype->ndims;
    dims = new int[ndims];
    std::copy(prototype->dims, prototype->dims + ndims, dims);
    atdims = prototype->atdims;
    natdims = prototype->natdims;
}

Vector::Vector(const std::shared_ptr<RawVector>& val, const Vector* prototype, int new_dim_access) {
    value = val;
    ndims = prototype->ndims;
    dims = new int[ndims];
    std::copy(prototype->dims, prototype->dims + ndims, dims);
    atdims = new int[prototype->natdims + 1];
    std::copy(prototype->atdims, prototype->atdims + prototype->natdims, atdims);
    atdims[prototype->natdims] = new_dim_access;
    natdims = prototype->natdims + 1;
}

Vector::Vector(const std::shared_ptr<RawVector>& val, int size1, int size2) {
    value = val;
    dims = new int[2];
    dims[0] = size1;
    dims[1] = size2;
    ndims = 2;
    atdims = nullptr;
    natdims = 0;
}

Vector::~Vector() {
    bool shouldUnlock = value->lockable;
    value->lock();
    value->lockable -= 1;
    if(shouldUnlock)
        value->unsafeUnlock();
    delete[] dims;
}

int Vector::getType() const {
    return VECTOR;
}

std::string Vector::toString() const {
    value->lock();
    std::string result = "[";
    for (int i = 0; i < value->size; i++) {
        if (result.size() > 1) {
            result += ", ";
        }
        result += std::to_string(value->data[i]);
    }
    value->unlock();
    return result + "]";
}

std::shared_ptr<RawVector> Vector::getValue() const {
    return value;
}

Data* Vector::shallowCopy() const {
    value->lock();
    Vector* ret = new Vector(value, this);
    bool shouldUnlock = value->lockable;
    value->lockable += 1;
    if(shouldUnlock)
        value->unsafeUnlock();
    return ret;
}


Data* Vector::implement(const OperationType operation, BuiltinArgs* args)  {
    if(operation==AT && args->size==2 && args->arg0->getType()==VECTOR && args->arg1->getType()==INT) {
        if(ndims==natdims+1) {
            int index = ((Integer*)args->arg1)->getValue();
            value->lock();
            if(natdims)
                for(int i=0;i<natdims;i++) {
                    index *= dims[i+1];
                    index += atdims[i];
                }
            if(index < 0 || index>=value->size) {
                std::cerr << "Vector index "<<index<<" out of range [0,"<<value->size<<")\n";
                value->unlock();
                exit(1);
                return nullptr;
            }
            double val = value->data[index];
            value->unlock();
            FLOAT_RESULT(val);
        }
        else {
            value->lock();
            int index = ((Integer*)args->arg1)->getValue();
            if(natdims)
                for(int i=0;i<natdims;i++) {
                    index *= dims[i+1];
                    index += atdims[i];
                }
            if(index < 0 || index>=value->size) {
                std::cerr << "Vector index "<<index<<" out of range [0,"<<value->size<<")\n";
                value->unlock();
                exit(1);
                return nullptr;
            }
            Vector* ret = new Vector(value, this, index);
            bool shouldUnlock = value->lockable;
            value->lockable += 1;
            if(shouldUnlock)
                value->unsafeUnlock();
            return ret;
        }
    }
    if(operation==PUT && args->size==3 && args->arg0->getType()==VECTOR && args->arg1->getType()==INT && args->arg2->getType()==FLOAT) {
        value->lock();
        int index = ((Integer*)args->arg1)->getValue();
            for(int i=0;i<natdims;i++) {
                index *= dims[i+1];
                index += atdims[i];
            }
        double newValue = ((BFloat*)args->arg2)->getValue();
        if(index < 0 || index>=value->size) {
            std::cerr << "Index "<<index<<" out of range [0,"<<value->size<<")\n";
            exit(1);
        }
        value->data[index] = newValue;
        value->unlock();
        return nullptr;
    }
    if(operation==PUT && args->size==3 && args->arg0->getType()==VECTOR && args->arg1->getType()==INT && args->arg2->getType()==INT) {
        value->lock();
        int index = ((Integer*)args->arg1)->getValue();
            for(int i=0;i<natdims;i++) {
                index *= dims[i+1];
                index += atdims[i];
            }
        int newValue = ((Integer*)args->arg2)->getValue();
        if(index < 0 || index>=value->size)  {
            std::cerr << "Index "<<index<<" out of range [0,"<<value->size<<")\n";
            exit(1);
        }
        else
            value->data[index] = newValue;
        value->unlock();
        return nullptr;
    }
    if(operation==SUM && args->size==1) {
        value->lock();
        if(value->size==0) {
            value->unlock();
            std::cout << "Cannot apply sum on empty vector\n";
            return nullptr;
        }
        double ret = 0;
        for(int i=0;i<value->size;i++)
            ret += value->data[i];
        value->unlock();
        FLOAT_RESULT(ret);
    }
    if(operation==MAX && args->size==1) {
        value->lock();
        if(value->size==0) {
            value->unlock();
            std::cout << "Cannot apply max on empty vector\n";
            return nullptr;
        }
        double ret = value->data[0];
        for(int i=1;i<value->size;i++) {
            double element = value->data[i];
            if(element>ret)
                ret = element;
        }
        value->unlock();
        FLOAT_RESULT(ret);
    }
    if(operation==MIN && args->size==1) {
        value->lock();
        if(value->size==0) {
            value->unlock();
            std::cout << "Cannot apply min on empty vector\n";
            return nullptr;
        }
        double ret = value->data[0];
        for(int i=1;i<value->size;i++) {
            double element = value->data[i];
            if(element<ret)
                ret = element;
        }
        value->unlock();
        FLOAT_RESULT(ret);
    }
    if(args->size==2 && args->arg0->getType()==VECTOR && args->arg1->getType()==VECTOR) {
        Vector* a1 = (Vector*)args->arg0;
        Vector* a2 = (Vector*)args->arg1;
        bool order = true;
        if(order){
            a1->value->lock();
            if(a1->value!=a2->value)
                a2->value->lock();
        }
        else {
            if(a1->value!=a2->value)
                a2->value->lock();
            a1->value->lock();
        }
        int n = a1->value->size;
        if(a2->value->size!=n && operation!=MMUL) {
            std::cerr << "Vectors of different sizes: "+std::to_string(a1->value->size)+" vs "+std::to_string(a2->value->size)+"\n";
            exit(1);
            return nullptr;
        }
        
        if(operation==MMUL && (a1->ndims!=2 || a2->ndims!=2 || a1->dims[1]!=a2->dims[0])) {
            std::cerr << "Cannot multiply given matrices\n";
            exit(1);
            return nullptr;
        }
        std::shared_ptr<RawVector> rawret = operation==MMUL?std::make_shared<RawVector>(a1->dims[0]*a2->dims[1]):std::make_shared<RawVector>(a1->value->size);
        Vector* retret = operation==MMUL?new Vector(rawret, a1->dims[0], a2->dims[1]):new Vector(rawret, this);
        double* ret = rawret->data;
        double* v1 = a1->value->data;
        double* v2 = a2->value->data;
        if(operation==EQ)
            for(int i=0;i<n;i++)
                ret[i] = v1[i]==v2[i];
        if(operation==NEQ)
            for(int i=0;i<n;i++)
                ret[i] = v1[i]!=v2[i];
        if(operation==LT)
            for(int i=0;i<n;i++)
                ret[i] = v1[i]<v2[i];
        if(operation==LE)
            for(int i=0;i<n;i++)
                ret[i] = v1[i]<=v2[i];
        if(operation==GT)
            for(int i=0;i<n;i++)
                ret[i] = v1[i]>v2[i];
        if(operation==GE)
            for(int i=0;i<n;i++)
                ret[i] = v1[i]>=v2[i];
        if(operation==ADD)
            for(int i=0;i<n;i++)
                ret[i] = v1[i]+v2[i];
        if(operation==SUB)
            for(int i=0;i<n;i++)
                ret[i] = v1[i]-v2[i];
        if(operation==POW)
            for(int i=0;i<n;i++)
                ret[i] = pow(v1[i], v2[i]);
        if(operation==MUL)
            for(int i=0;i<n;i++)
                ret[i] = v1[i]*v2[i];
        if(operation==DIV)
            for(int i=0;i<n;i++)
                ret[i] = v1[i]/v2[i];
        if(operation==MMUL) {
            for(int k=0;k<rawret->size;k++)
                ret[k] = 0;
            int n1 = a1->dims[0];
            int n2 = a2->dims[1];
            int cols1 = a1->dims[1];
            int cols2 = a2->dims[1];
            int kmax = a1->dims[1];
            for(int k=0;k<kmax;k++) {
                for(int i1=0;i1<n1;i1++)
                    for(int i2=0;i2<n2;i2++) {
                        ret[POS(cols1, i1, i2)] += v1[POS(cols2, i1, k)]*v2[POS(cols2, k, i2)];
                        //std::cout << i1 << " "<< i2 << " " << k <<"   "<<ret[POS(n1, i1, i2)]<<"\n";
                    }
            }
        }
        if(order){
            if(a1->value!=a2->value)
                a2->value->unlock();
            a1->value->unlock();
        }
        else {
            a1->value->unlock();
            if(a1->value!=a2->value)
                a2->value->unlock();
        }
        return retret;
    }
    if(args->size==2 
        && (args->arg0->getType()==VECTOR || args->arg0->getType()==FLOAT || args->arg0->getType()==INT) 
        && (args->arg1->getType()==VECTOR || args->arg1->getType()==FLOAT || args->arg1->getType()==INT)
        && ((args->arg0->getType()==VECTOR)!=(args->arg1->getType()==VECTOR))) { 
        std::shared_ptr<RawVector> vec = args->arg0->getType()==VECTOR?((Vector*)args->arg0)->getValue():((Vector*)args->arg1)->getValue();
        Data* uncastedother = args->arg0->getType()==VECTOR?args->arg1:args->arg0;
        double v = uncastedother->getType()==INT?((Integer*)uncastedother)->getValue():((BFloat*)uncastedother)->getValue();
        vec->lock();
        int n = vec->size;
        std::shared_ptr<RawVector> rawret = std::make_shared<RawVector>(n);
        double* ret = rawret->data;
        double* dat = value->data;
        bool left = args->arg0->getType()==VECTOR;
        if(operation==EQ) 
            for(int i=0;i<n;i++)
                ret[i] = dat[i]==v;
        if(operation==NEQ)
            for(int i=0;i<n;i++)
                ret[i] = dat[i]!=v;
        if(operation==ADD)
            for(int i=0;i<n;i++)
                ret[i] = dat[i]+v;
        if(operation==SUB){
            if(left)
                for(int i=0;i<n;i++)
                    ret[i] = dat[i]-v;
            else
                for(int i=0;i<n;i++)
                    ret[i] = v-dat[i];
        }
        if(operation==MUL)
            for(int i=0;i<n;i++)
                ret[i] = dat[i]*v;
        if(operation==DIV){
            if(left)
                for(int i=0;i<n;i++)
                    ret[i] = dat[i]/v;
            else
                for(int i=0;i<n;i++)
                    ret[i] = v/dat[i];
        }
        if(operation==POW){
            if(left)
                for(int i=0;i<n;i++)
                    ret[i] = pow(dat[i], v);
            else
                for(int i=0;i<n;i++)
                    ret[i] = pow(v, dat[i]);
        }
        vec->unlock();
        return new Vector(rawret, this);
    }
    if(operation==TOCOPY && args->size==1 && args->arg0->getType()==VECTOR)
        return shallowCopy();
    if(operation==LEN && args->size==1 && args->arg0->getType()==VECTOR) {
        INT_RESULT(value->size);
    }
    if(operation==TOSTR && args->size==1 && args->arg0->getType()==VECTOR) {
        std::string ret = toString();
        return new BString(ret);
    }
    if(args->size==1 && operation==TOITER) {
        Data* ret = new Iterator(std::make_shared<IteratorContents>(new Vector(value, this)));
        return ret;
    }
    if(operation==SHAPE && args->size==1 && args->arg0->getType()==VECTOR) {
        int n = ndims;
        Vector* ret = new Vector(n);
        for(int i=0;i<n;i++)
            ret->value->data[i] = dims[i];
        return ret;
    }
    throw Unimplemented();
}
