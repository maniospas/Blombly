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
    virtual ~Data() = default;
    static std::shared_ptr<Data> run(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        std::shared_ptr<Data> ret;
        for(const std::shared_ptr<Data>& implementer : all)
            try {
                ret = implementer->implement(operation, all);
                break;
            } 
            catch(Unimplemented) { // TODO: catch only Unimplemented exceptions
            }
        if(ret==nullptr)
            std::cerr << "No valid implementation of "+operation+" for any of its arguments" << std::endl;
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
    int getThread() const {return thread;}
};


class Integer : public Data {
private:
    int value;
public:
    Integer(int val) : value(val) {}
    std::string getType() const override {return "int";}
    std::string toString() const override {return std::to_string(value);}
    int getValue() const {return value;}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==2 && all[0]->getType()=="int" && all[1]->getType()=="int") {
            int v1 = std::static_pointer_cast<Integer>(all[0])->getValue();
            int v2 = std::static_pointer_cast<Integer>(all[1])->getValue();
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
};

class Float : public Data {
private:
    float value;
public:
    Float(float val) : value(val) {}
    std::string getType() const override {return "float";}
    std::string toString() const override {return std::to_string(value);}
    float getValue() const {return value;}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==2 
            && (all[0]->getType()=="float" || all[0]->getType()=="int") 
            && (all[1]->getType()=="float" || all[1]->getType()=="int")) { 
            float v1 = all[0]->getType()=="int"?std::static_pointer_cast<Integer>(all[0])->getValue():std::static_pointer_cast<Float>(all[0])->getValue();
            float v2 = all[1]->getType()=="int"?std::static_pointer_cast<Integer>(all[1])->getValue():std::static_pointer_cast<Float>(all[1])->getValue();
            float res;
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


class BString : public Data {
private:
    std::string value;
public:
    BString(const std::string& val) : value(val) {}
    std::string getType() const override {return "string";}
    std::string toString() const override {return value;}
};


class Code : public Data {
private:
    int start, end;
public:
    Code(int startAt, int endAt) : start(startAt), end(endAt) {}
    std::string getType() const override {return "code";}
    std::string toString() const override {return "code from "+std::to_string(start)+" to "+std::to_string(end);}
    int getStart() const {return start;}
    int getEnd() const {return end;}
};