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


class Unimplemented {
    /**
     * An object of this type is thrown as an exception when trying to
     * implement overloaded Data methods.
    */
public:
    Unimplemented() {
    }
};



class Data {
    /**
     * This abstract class represents data stored in the memory.
    */
public:
    bool isMutable = true; // mutable means that it cannot be shared
    virtual std::string toString() const = 0;
    virtual std::string getType() const = 0;
    virtual std::shared_ptr<Data> shallowCopy() const = 0;
    virtual ~Data() = default;
    static std::shared_ptr<Data> run(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        std::shared_ptr<Data> ret;
        for(const std::shared_ptr<Data>& implementer : all)
            try {
                ret = implementer->implement(operation, all);
                break;
            } 
            catch(Unimplemented) { // TODO: catch only Unimplemented exceptions
                std::string err = "No valid builtin implementation for this method: "+operation+"(";
                int i = 0;
                for(const std::shared_ptr<Data>& arg : all) {
                    if(i)
                        err += ",";
                    err += arg->getType();
                    i++;
                }
                err += ")";
                std::cerr << err << std::endl;
            }
        return ret;
    }
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        throw Unimplemented();
    }
};

class Future: public Data {
private:
    int thread;
public:
    Future(int threadid): thread(threadid) {}
    std::string getType() const override {return "future";}
    std::string toString() const override {return "future";}
    std::shared_ptr<Data> shallowCopy() const override {throw Unimplemented();}
    int getThread() const {return thread;}
};


class Boolean : public Data {
private:
    bool value;
public:
    Boolean(bool val) : value(val) {}
    std::string getType() const override {return "bool";}
    std::string toString() const override {return value?"true":"false";}
    bool getValue() const {return value;}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<Boolean>(value);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()=="bool" && operation=="copy")
            return std::make_shared<Boolean>(value);
        if(all.size()==2 && all[0]->getType()=="bool" && all[1]->getType()=="bool") {
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
    std::string getType() const override {return "int";}
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
    std::string getType() const override {return "float";}
    std::string toString() const override {return std::to_string(value);}
    double getValue() const {return value;}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<Float>(value);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()=="float" && operation=="copy")
            return std::make_shared<Float>(value);
        if(all.size()==2 
            && (all[0]->getType()=="float" || all[0]->getType()=="int") 
            && (all[1]->getType()=="float" || all[1]->getType()=="int")) { 
            double v1 = all[0]->getType()=="int"?std::static_pointer_cast<Integer>(all[0])->getValue():std::static_pointer_cast<Float>(all[0])->getValue();
            double v2 = all[1]->getType()=="int"?std::static_pointer_cast<Integer>(all[1])->getValue():std::static_pointer_cast<Float>(all[1])->getValue();
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


class Vector : public Data {
private:
    std::shared_ptr<RawVector> value;
public:
    Vector(int size) {
        value = std::make_shared<RawVector>(size);    
    }
    Vector(std::shared_ptr<RawVector> val) {
        value=val;
    }
    std::string getType() const override {return "vec";}
    std::string toString() const override {return "vector of size "+std::to_string(value->size);}
    std::shared_ptr<RawVector> getValue() const {return value;}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<Vector>(value);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()=="vec" && operation=="copy")
            return std::make_shared<Vector>(value);
        if(all.size()==2 && all[0]->getType()=="vec" && all[1]->getType()=="int" && operation=="at") {
            value->lock();
            int index = std::static_pointer_cast<Integer>(all[1])->getValue();
            if(index < 0 || index>=value->size) {
                std::cerr << "Index out of range\n";
                value->unlock();
                return nullptr;
            }
            double val = value->data[index];
            value->unlock();
            return std::make_shared<Float>(val);
        }
        if(all.size()==3 && all[0]->getType()=="vec" && all[1]->getType()=="int" && all[2]->getType()=="int" && operation=="put") {
            value->lock();
            int index = std::static_pointer_cast<Integer>(all[1])->getValue();
            int newValue = std::static_pointer_cast<Integer>(all[2])->getValue();
            if(index < 0 || index>=value->size) 
                std::cerr << "Index out of range\n";
            else
                value->data[index] = newValue;
            value->unlock();
            return nullptr;
        }
        if(all.size()==3 && all[0]->getType()=="vec" && all[1]->getType()=="int" && all[2]->getType()=="float" && operation=="put") {
            value->lock();
            int index = std::static_pointer_cast<Integer>(all[1])->getValue();
            double newValue = std::static_pointer_cast<Float>(all[2])->getValue();
            if(index < 0 || index>=value->size) 
                std::cerr << "Index out of range\n";
            value->data[index] = newValue;
            value->unlock();
            return nullptr;
        }
        if(all.size()==2 && all[0]->getType()=="vec" && all[1]->getType()=="vec") {
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
            if(a2->value->size!=n) {
                std::cerr << "Vectors of different sizes: "+std::to_string(n)+" vs "+std::to_string(a1->value->size)+"\n";
                return nullptr;
            }
            std::shared_ptr<RawVector> rawret = std::make_shared<RawVector>(n);
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
            if(order){
                a2->value->unlock();
                a1->value->unlock();
            }
            else {
                a1->value->unlock();
                a2->value->unlock();
            }
            return std::make_shared<Vector>(rawret);
        }
        if(all.size()==2 
            && (all[0]->getType()=="vec" || all[0]->getType()=="float" || all[1]->getType()=="int") 
            && (all[1]->getType()=="vec" || all[1]->getType()=="float" || all[1]->getType()=="int")
            && ((all[0]->getType()=="vec")!=(all[1]->getType()=="vec"))) { 
            std::shared_ptr<RawVector> vec = all[0]->getType()=="vec"?std::static_pointer_cast<Vector>(all[0])->getValue():std::static_pointer_cast<Vector>(all[1])->getValue();
            std::shared_ptr<Data> uncastedother = all[0]->getType()=="vec"?all[1]:all[0];
            double v = uncastedother->getType()=="int"?std::static_pointer_cast<Integer>(uncastedother)->getValue():std::static_pointer_cast<Float>(uncastedother)->getValue();
            vec->lock();
            int n = vec->size;
            std::shared_ptr<RawVector> rawret = std::make_shared<RawVector>(n);
            double* ret = rawret->data;
            double* dat = value->data;
            bool left = all[0]->getType()=="vec";
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
            return std::make_shared<Vector>(rawret);
        }
        throw Unimplemented();
    }
};



class BString : public Data {
private:
    std::string value;
public:
    BString(const std::string& val) : value(val) {}
    std::string getType() const override {return "string";}
    std::string toString() const override {return value;}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<BString>(value);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()=="string" && operation=="copy")
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
    std::string getType() const override {return "code";}
    std::string toString() const override {return "code from "+std::to_string(start)+" to "+std::to_string(end);}
    int getStart() const {return start;}
    int getEnd() const {return end;}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<Code>(start, end, declarationMemory);}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()=="code" && operation=="copy")
            return std::make_shared<Code>(start, end, declarationMemory);
        throw Unimplemented();
    }
    std::shared_ptr<Memory>& getDeclarationMemory() {
        return declarationMemory;
    }
};


std::shared_ptr<Data> Integer::implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
    if(all.size()==1 && all[0]->getType()=="int" && operation=="copy")
        return std::make_shared<Integer>(value);
    if(all.size()==1 && all[0]->getType()=="int" && operation=="Vector")
        return std::make_shared<Vector>(value);
    if(all.size()==2 && all[0]->getType()=="int" && all[1]->getType()=="int") {
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
            res = v1 / v2;
        if(operation=="mod")
            res = v1 % v2;
        if(operation=="pow")
            res = pow(v1, v2);
        return std::make_shared<Integer>(res);
    }
    throw Unimplemented();
}