#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <stack>
#include <map>
#include <sstream>
#include <memory> 
#include <cstdlib>
#include <algorithm>
#include <unordered_set>  
#include <pthread.h>
#include <queue>
#include <atomic>
#include <functional>
#include <thread>
#include <cmath>
#include <future>

# define POS(v, i, j) (i*v+j)

class Unimplemented {
    /**
     * An object of this type is thrown as an exception when trying to
     * implement overloaded Data methods.
    */
public:
    Unimplemented() {
    }
};




enum Datatype {FUTURE, BOOL, INT, FLOAT, VECTOR, LIST, STRING, CODE, STRUCT};
static const char *datatypeName[] = { 
    "future", 
    "bool", 
    "int", 
    "float", 
    "vector", 
    "list",
    "string",
    "code",
    "struct"
};



class Data {
    /**
     * This abstract class represents data stored in the memory.
    */
public:
    bool isMutable = true; // mutable means that it cannot be shared
    virtual std::string toString() const = 0;
    virtual int getType() const = 0;
    virtual std::shared_ptr<Data> shallowCopy() const = 0;
    virtual bool couldBeShallowCopy(std::shared_ptr<Data> data) {
        return false;
    }
    virtual ~Data() = default;
    static std::shared_ptr<Data> run(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        std::shared_ptr<Data> ret;
        for(const std::shared_ptr<Data>& implementer : all)
            try {
                return implementer->implement(operation, all);
            } 
            catch(Unimplemented) { // TODO: catch only Unimplemented exceptions
            }
        std::string err = "No valid builtin implementation for this method: "+operation+"(";
        int i = 0;
        for(const std::shared_ptr<Data>& arg : all) {
            if(i)
                err += ",";
            err += datatypeName[arg->getType()];
            i++;
        }
        err += ")";
        std::cerr << err << std::endl;
        return nullptr;
    }
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        throw Unimplemented();
    }
};


/*
class Future: public Data {
private:
    int thread;
public:
    Future(int threadid): thread(threadid) {}
    int getType() const override {return FUTURE;}
    std::string toString() const override {return "future";}
    std::shared_ptr<Data> shallowCopy() const override {throw Unimplemented();}
    int getThread() const {return thread;}
};
*/

class FutureData {
public:
    std::future<std::shared_ptr<Data>> thread;
    FutureData(std::future<std::shared_ptr<Data>>&& threadid):thread(std::move(threadid)) {}
};

class Future: public Data {
private:
    std::shared_ptr<FutureData> data;
public:
    Future(std::shared_ptr<FutureData> data):data(data) {}
    int getType() const override {return FUTURE;}
    std::string toString() const override {return "future";}
    std::shared_ptr<Data> shallowCopy() const override {
        return std::make_shared<Future>(data);
    }//throw Unimplemented();}
    std::shared_ptr<Data> getResult() {
        return data->thread.get();
    }
};


class Boolean : public Data {
private:
    bool value;
public:
    Boolean(bool val) : value(val) {}
    int getType() const override {return BOOL;}
    std::string toString() const override {return value?"true":"false";}
    bool getValue() const {return value;}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<Boolean>(value);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()==BOOL && operation=="copy")
            return std::make_shared<Boolean>(value);
        if(all.size()==2 && all[0]->getType()==BOOL && all[1]->getType()==BOOL) {
            bool v1 = std::static_pointer_cast<Boolean>(all[0])->getValue();
            bool v2 = std::static_pointer_cast<Boolean>(all[1])->getValue();
            bool res;
            if(operation=="and")
                res = v1 && v2;
            if(operation=="or")
                res = v1 || v2;
            if(operation=="eq")
                res = v1 == v2;
            if(operation=="neq")
                res = v1 != v2;
            return std::make_shared<Boolean>(res);
        }
        throw Unimplemented();
    }
};

class Integer : public Data {
private:
    int value;
public:
    Integer(int val) : value(val) {}
    int getType() const override {return INT;}
    std::string toString() const override {return std::to_string(value);}
    int getValue() const {return value;}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<Integer>(value);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all);
};

class Float : public Data {
private:
    double value;
public:
    Float(double val) : value(val) {}
    int getType() const override {return FLOAT;}
    std::string toString() const override {return std::to_string(value);}
    double getValue() const {return value;}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<Float>(value);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()==FLOAT && operation=="copy")
            return std::make_shared<Float>(value);
        if(all.size()==1 && all[0]->getType()==FLOAT && operation=="int")
            return std::make_shared<Integer>((int)value);
        if(all.size()==2 
            && (all[0]->getType()==FLOAT|| all[0]->getType()==INT) 
            && (all[1]->getType()==FLOAT || all[1]->getType()==INT)) { 
            double v1 = all[0]->getType()==INT?std::static_pointer_cast<Integer>(all[0])->getValue():std::static_pointer_cast<Float>(all[0])->getValue();
            double v2 = all[1]->getType()==INT?std::static_pointer_cast<Integer>(all[1])->getValue():std::static_pointer_cast<Float>(all[1])->getValue();
            if(operation=="eq")
                return std::make_shared<Boolean>(v1 == v2);
            if(operation=="neq")
                return std::make_shared<Boolean>(v1 != v2);
            if(operation=="lt")
                return std::make_shared<Boolean>(v1 < v2);
            if(operation=="le")
                return std::make_shared<Boolean>(v1 <= v2);
            if(operation=="gt")
                return std::make_shared<Boolean>(v1 > v2);
            if(operation=="ge")
                return std::make_shared<Boolean>(v1 >= v2);
            double res;
            if(operation=="add")
                res = v1 + v2;
            if(operation=="sub")
                res = v1 - v2;
            if(operation=="mul")
                res = v1 * v2;
            if(operation=="div")
                res = v1 / v2;
            if(operation=="pow")
                res = pow(v1, v2);
            return std::make_shared<Float>(res);
        }
        throw Unimplemented();
    }
};


class RawVector {
private:
    pthread_mutex_t memoryLock; 
public:
    double* data;
    int size;
    RawVector(int siz) {
        size = siz;
        data = new double[size];
        if (pthread_mutex_init(&memoryLock, NULL) != 0) 
            std::cerr << "Failed to create a mutex for vector read/write" << std::endl;
    }
    ~RawVector() {
        delete data;
    }
    void lock() {
        pthread_mutex_lock(&memoryLock);
    }
    void unlock(){
        pthread_mutex_unlock(&memoryLock);
    }
};


class ListContents {
private:
    pthread_mutex_t memoryLock; 
public:
    std::vector<std::shared_ptr<Data>> contents;
    ListContents() {
        if (pthread_mutex_init(&memoryLock, NULL) != 0) 
            std::cerr << "Failed to create a mutex for list read/write" << std::endl;
    }
    void lock() {
        pthread_mutex_lock(&memoryLock);
    }
    void unlock(){
        pthread_mutex_unlock(&memoryLock);
    }
};


class List : public Data {
public:
    std::shared_ptr<ListContents> contents;
    List() {
        contents = std::make_shared<ListContents>();
    }
    List(std::shared_ptr<ListContents> cont): contents(cont) {
    }
    
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<List>(contents);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all);

    int getType() const override {return LIST;}
    std::string toString() const override {
        contents->lock();
        std::string ret = "[";
        for(const auto& element : contents->contents) {
            if(ret.size()>1)
                ret += ", ";
            ret += element->toString();
        }
        contents->unlock();
        return ret+"]";
    }
};


class Vector : public Data {
public:
    int* atdims;
    int natdims;
    int* dims;
    int ndims;
    std::shared_ptr<RawVector> value;
    Vector(int size) {
        value = std::make_shared<RawVector>(size);
        dims = new int[1];
        dims[0] = size;
        ndims = 1;    
        atdims = nullptr;
        natdims = 0;
    }
    Vector(int size, bool setToZero) {
        value = std::make_shared<RawVector>(size);  
        dims = new int[1];
        dims[0] = size;
        ndims = 1;
        if(setToZero)
            for(int i=0;i<size;i++)
                value->data[i] = 0;    
        atdims = nullptr;
        natdims = 0;
    }
    Vector(int size1, int size2) {
        value = std::make_shared<RawVector>(size1*size2);
        dims = new int[2];
        dims[0] = size1;
        dims[1] = size2;
        ndims = 2;     
        atdims = nullptr;
        natdims = 0; 
    }
    Vector(int size1, int size2, bool setToZero) {
        value = std::make_shared<RawVector>(size1*size2);  
        dims = new int[2];
        dims[0] = size1;
        dims[1] = size2;
        ndims = 2;   
        if(setToZero)
            for(int i=0;i<value->size;i++)
                value->data[i] = 0; 
        atdims = nullptr;
        natdims = 0;    
    }
    virtual ~Vector() {
        delete dims;
    }
    Vector(std::shared_ptr<RawVector> val, const Vector* prototype) { // todo: change prototype to shared int*
        value=val;
        ndims = prototype->ndims;
        dims = new int[prototype->ndims];
        for(int i=0;i<ndims;i++)
            dims[i] = prototype->dims[i];
        atdims = prototype->atdims;
        natdims = prototype->natdims;    
    }
    Vector(std::shared_ptr<RawVector> val, const Vector* prototype, const int new_dim_access) { // todo: change prototype to shared int*
        value=val;
        ndims = prototype->ndims;
        dims = new int[prototype->ndims];
        for(int i=0;i<ndims;i++)
            dims[i] = prototype->dims[i];
        atdims = new int[prototype->natdims+1];
        for(int i=0;i<prototype->natdims;i++)
            atdims[i] = prototype->atdims[i];
        atdims[prototype->natdims] = new_dim_access;
        natdims = prototype->natdims+1;    
    }
    Vector(std::shared_ptr<RawVector> val, int size1, int size2) {
        value=val;
        dims = new int[2];
        dims[0] = size1;
        dims[1] = size2;
        ndims = 2;   
        atdims = nullptr;
        natdims = 0;    
    }
    int getType() const override {return VECTOR;}
    std::string toString() const override {
        value->lock();
        std::string ret = "[";
        for(int i=0;i<value->size;i++) {
            if(ret.size()>1)
                ret += ", ";
            ret += std::to_string(value->data[i]);
        }
        value->unlock();
        return ret+"]";
    }
    std::shared_ptr<RawVector> getValue() const {return value;}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<Vector>(value, this);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all);
};


class BString : public Data {
private:
    std::string value;
public:
    BString(const std::string& val) : value(val) {}
    int getType() const override {return STRING;}
    std::string toString() const override {return value;}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<BString>(value);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()==STRING && operation=="copy")
            return std::make_shared<BString>(value);
        throw Unimplemented();
    }
};

class Memory;

class Code : public Data {
private:
    int start, end;
    std::shared_ptr<Memory> declarationMemory;
public:
    Code(int startAt, int endAt, std::shared_ptr<Memory> declMemory) : start(startAt), end(endAt) {declarationMemory = declMemory;}
    int getType() const override {return CODE;}
    std::string toString() const override {return "code from "+std::to_string(start)+" to "+std::to_string(end);}
    int getStart() const {return start;}
    int getEnd() const {return end;}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<Code>(start, end, declarationMemory);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()==CODE && operation=="copy")
            return std::make_shared<Code>(start, end, declarationMemory);
        throw Unimplemented();
    }
    std::shared_ptr<Memory>& getDeclarationMemory() {
        return declarationMemory;
    }
};


std::shared_ptr<Data> Integer::implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
    if(all.size()==1 && all[0]->getType()==INT && operation=="copy")
        return std::make_shared<Integer>(std::static_pointer_cast<Integer>(all[0])->getValue());
    if(all.size()==1 && all[0]->getType()==INT && operation=="Vector")
        return std::make_shared<Vector>(std::static_pointer_cast<Integer>(all[0])->getValue(), true);
    if(all.size()==2 && all[0]->getType()==INT && all[1]->getType()==INT && operation=="Matrix")
        return std::make_shared<Vector>(std::static_pointer_cast<Integer>(all[0])->getValue(), std::static_pointer_cast<Integer>(all[1])->getValue(), true);
    if(all.size()==2 && all[0]->getType()==INT && all[1]->getType()==INT) {
        int v1 = std::static_pointer_cast<Integer>(all[0])->getValue();
        int v2 = std::static_pointer_cast<Integer>(all[1])->getValue();
        if(operation=="eq")
            return std::make_shared<Boolean>(v1 == v2);
        if(operation=="neq")
            return std::make_shared<Boolean>(v1 != v2);
        if(operation=="lt")
            return std::make_shared<Boolean>(v1 < v2);
        if(operation=="le")
            return std::make_shared<Boolean>(v1 <= v2);
        if(operation=="gt")
            return std::make_shared<Boolean>(v1 > v2);
        if(operation=="ge")
            return std::make_shared<Boolean>(v1 >= v2);
        int res;
        if(operation=="add")
            res = v1 + v2;
        if(operation=="sub")
            res = v1 - v2;
        if(operation=="mul")
            res = v1 * v2;
        if(operation=="div")
            return std::make_shared<Float>(v1/(float)v2);
            //res = v1 / v2;
        if(operation=="mod")
            res = v1 % v2;
        if(operation=="pow")
            res = pow(v1, v2);
        return std::make_shared<Integer>(res);
    }
    throw Unimplemented();
}


std::shared_ptr<Data> Vector::implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
    if(all.size()==1 && all[0]->getType()==VECTOR && operation=="copy")
        return std::make_shared<Vector>(value, this);
    if(all.size()==1 && all[0]->getType()==VECTOR && operation=="len")
        return std::make_shared<Integer>(value->size);
    if(all.size()==1 && all[0]->getType()==VECTOR && operation=="repr") {
        std::string ret = toString();
        return std::make_shared<BString>(ret);
    }
    if(all.size()==1 && all[0]->getType()==VECTOR && operation=="shape") {
        int n = ndims;
        std::shared_ptr<Vector> ret = std::make_shared<Vector>(n);
        for(int i=0;i<n;i++)
            ret->value->data[i] = dims[i];
        return ret;
    }
    if(all.size()==2 && all[0]->getType()==VECTOR && all[1]->getType()==INT && operation=="at") {
        if(ndims==natdims+1) {
            value->lock();
            int index = std::static_pointer_cast<Integer>(all[1])->getValue();
            for(int i=0;i<natdims;i++) {
                index *= dims[i+1];
                index += atdims[i];
            }
            if(index < 0 || index>=value->size) {
                std::cerr << "Index out of range\n";
                value->unlock();
                return nullptr;
            }
            int pos = 0;
            double val = value->data[index];
            value->unlock();
            return std::make_shared<Float>(val);
        }
        else {
            value->lock();
            int index = std::static_pointer_cast<Integer>(all[1])->getValue();
            for(int i=0;i<natdims;i++) {
                index *= dims[i+1];
                index += atdims[i];
            }
            if(index < 0 || index>=value->size) {
                std::cerr << "Index out of range\n";
                value->unlock();
                return nullptr;
            }
            std::shared_ptr<Vector> ret = std::make_shared<Vector>(value, this, index);
            value->unlock();
            return ret;
        }
    }
    if(all.size()==3 && all[0]->getType()==VECTOR && all[1]->getType()==INT && all[2]->getType()==INT && operation=="put") {
        value->lock();
        int index = std::static_pointer_cast<Integer>(all[1])->getValue();
            for(int i=0;i<natdims;i++) {
                index *= dims[i+1];
                index += atdims[i];
            }
        int newValue = std::static_pointer_cast<Integer>(all[2])->getValue();
        if(index < 0 || index>=value->size) 
            std::cerr << "Index out of range\n";
        else
            value->data[index] = newValue;
        value->unlock();
        return nullptr;
    }
    if(all.size()==3 && all[0]->getType()==VECTOR && all[1]->getType()==INT && all[2]->getType()==FLOAT && operation=="put") {
        value->lock();
        int index = std::static_pointer_cast<Integer>(all[1])->getValue();
            for(int i=0;i<natdims;i++) {
                index *= dims[i+1];
                index += atdims[i];
            }
        double newValue = std::static_pointer_cast<Float>(all[2])->getValue();
        if(index < 0 || index>=value->size) 
            std::cerr << "Index out of range\n";
        value->data[index] = newValue;
        value->unlock();
        return nullptr;
    }
    if(all.size()==2 && all[0]->getType()==VECTOR && all[1]->getType()==VECTOR) {
        std::shared_ptr<Vector> a1 = std::static_pointer_cast<Vector>(all[0]);
        std::shared_ptr<Vector> a2 = std::static_pointer_cast<Vector>(all[1]);
        bool order = true;
        if(order){
            a1->value->lock();
            a2->value->lock();
        }
        else {
            a2->value->lock();
            a1->value->lock();
        }
        int n = a1->value->size;
        if(a2->value->size!=n && operation!="mmul") {
            std::cerr << "Vectors of different sizes: "+std::to_string(a1->value->size)+" vs "+std::to_string(a2->value->size)+"\n";
            return nullptr;
        }
        
        if(operation=="mmul" && (a1->ndims!=2 || a2->ndims!=2 || a1->dims[1]!=a2->dims[0])) {
            std::cerr << "Cannot multiply given matrices\n";
            return nullptr;
        }
        std::shared_ptr<RawVector> rawret = operation=="mmul"?std::make_shared<RawVector>(a1->dims[0]*a2->dims[1]):std::make_shared<RawVector>(a1->value->size);
        std::shared_ptr<Vector> retret = operation=="mmul"?std::make_shared<Vector>(rawret, a1->dims[0], a2->dims[1]):std::make_shared<Vector>(rawret, this);
        double* ret = rawret->data;
        double* v1 = a1->value->data;
        double* v2 = a2->value->data;
        if(operation=="eq")
            for(int i=0;i<n;i++)
                ret[i] = v1[i]==v2[i];
        if(operation=="neq")
            for(int i=0;i<n;i++)
                ret[i] = v1[i]!=v2[i];
        if(operation=="lt")
            for(int i=0;i<n;i++)
                ret[i] = v1[i]<v2[i];
        if(operation=="le")
            for(int i=0;i<n;i++)
                ret[i] = v1[i]<=v2[i];
        if(operation=="gt")
            for(int i=0;i<n;i++)
                ret[i] = v1[i]>v2[i];
        if(operation=="ge")
            for(int i=0;i<n;i++)
                ret[i] = v1[i]>=v2[i];
        if(operation=="add")
            for(int i=0;i<n;i++)
                ret[i] = v1[i]+v2[i];
        if(operation=="sub")
            for(int i=0;i<n;i++)
                ret[i] = v1[i]-v2[i];
        if(operation=="pow")
            for(int i=0;i<n;i++)
                ret[i] = pow(v1[i], v2[i]);
        if(operation=="mul")
            for(int i=0;i<n;i++)
                ret[i] = v1[i]*v2[i];
        if(operation=="div")
            for(int i=0;i<n;i++)
                ret[i] = v1[i]/v2[i];
        if(operation=="mmul") {
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
            a2->value->unlock();
            a1->value->unlock();
        }
        else {
            a1->value->unlock();
            a2->value->unlock();
        }
        return retret;
    }
    if(all.size()==2 
        && (all[0]->getType()==VECTOR || all[0]->getType()==FLOAT || all[0]->getType()==INT) 
        && (all[1]->getType()==VECTOR || all[1]->getType()==FLOAT || all[1]->getType()==INT)
        && ((all[0]->getType()==VECTOR)!=(all[1]->getType()==VECTOR))) { 
        std::shared_ptr<RawVector> vec = all[0]->getType()==VECTOR?std::static_pointer_cast<Vector>(all[0])->getValue():std::static_pointer_cast<Vector>(all[1])->getValue();
        std::shared_ptr<Data> uncastedother = all[0]->getType()==VECTOR?all[1]:all[0];
        double v = uncastedother->getType()==INT?std::static_pointer_cast<Integer>(uncastedother)->getValue():std::static_pointer_cast<Float>(uncastedother)->getValue();
        vec->lock();
        int n = vec->size;
        std::shared_ptr<RawVector> rawret = std::make_shared<RawVector>(n);
        double* ret = rawret->data;
        double* dat = value->data;
        bool left = all[0]->getType()==VECTOR;
        if(operation=="eq") 
            for(int i=0;i<n;i++)
                ret[i] = dat[i]==v;
        if(operation=="neq")
            for(int i=0;i<n;i++)
                ret[i] = dat[i]!=v;
        if(operation=="add")
            for(int i=0;i<n;i++)
                ret[i] = dat[i]+v;
        if(operation=="sub"){
            if(left)
                for(int i=0;i<n;i++)
                    ret[i] = dat[i]-v;
            else
                for(int i=0;i<n;i++)
                    ret[i] = v-dat[i];
        }
        if(operation=="mul")
            for(int i=0;i<n;i++)
                ret[i] = dat[i]*v;
        if(operation=="div"){
            if(left)
                for(int i=0;i<n;i++)
                    ret[i] = dat[i]/v;
            else
                for(int i=0;i<n;i++)
                    ret[i] = v/dat[i];
        }
        if(operation=="pow"){
            if(left)
                for(int i=0;i<n;i++)
                    ret[i] = pow(dat[i], v);
            else
                for(int i=0;i<n;i++)
                    ret[i] = pow(v, dat[i]);
        }
        vec->unlock();
        return std::make_shared<Vector>(rawret, this);
    }
    throw Unimplemented();
}



std::shared_ptr<Data> List::implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
    if(all.size()==1 && all[0]->getType()==LIST && operation=="copy")
        return std::make_shared<List>(contents);
    if(all.size()==1 && all[0]->getType()==LIST && operation=="len")
        return std::make_shared<Integer>(contents->contents.size());
    if(all.size()==2 && all[0]->getType()==LIST && all[1]->getType()==INT && operation=="at") {
        contents->lock();
        int index = std::static_pointer_cast<Integer>(all[1])->getValue();
        if(index < 0 || index>=contents->contents.size()) {
            std::cerr << "Index out of range\n";
            contents->unlock();
            return nullptr;
        }
        std::shared_ptr<Data> ret = contents->contents[index];
        contents->unlock();
        return ret;
    }
    if(all.size()==2 && all[0]->getType()==LIST && operation=="push") {
        contents->lock();
        contents->contents.push_back(all[1]);
        contents->unlock();
        return all[0];
    }
    if(all.size()==2 && all[1]->getType()==LIST && operation=="pop") {
        contents->lock();
        contents->contents.push_back(all[0]);
        contents->unlock();
        return all[1];
    }
    if(all.size()==1 && all[0]->getType()==LIST && operation=="pop") {
        contents->lock();
        std::shared_ptr<Data> ret = contents->contents.size()?contents->contents[contents->contents.size()-1]:nullptr;
        if(contents->contents.size())
            contents->contents.pop_back();
        contents->unlock();
        return ret;
    }
    if(all.size()==1 && all[0]->getType()==LIST && operation=="pop") {
        contents->lock();
        std::shared_ptr<Data> ret = contents->contents.size()?contents->contents[contents->contents.size()-1]:nullptr;
        if(contents->contents.size())
            contents->contents.pop_back();
        contents->unlock();
        return ret;
    }
    if(all.size()==3 && all[0]->getType()==LIST && all[1]->getType()==INT && operation=="put") {
        contents->lock();
        int index = std::static_pointer_cast<Integer>(all[1])->getValue();
        int diff = index-contents->contents.size();
        for(int i=0;i<=diff;i++)
            contents->contents.push_back(nullptr);
        contents->contents[index] = all[2];
        contents->unlock();
        return nullptr;
    }
    throw Unimplemented();
}