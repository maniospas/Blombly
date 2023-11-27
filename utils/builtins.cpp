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
        if(ret==nullptr) {
            std::string err = "No valid builtin implementation for this method: "+operation+"(";
            int i = 0;
            for(const std::shared_ptr<Data>& arg : all) {
                err += arg->getType();
                if(i)
                    err += ",";
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
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()=="int" && operation=="copy")
            return std::make_shared<Integer>(value);
        if(all.size()==2 && all[0]->getType()=="int" && all[1]->getType()=="int") {
            int v1 = std::static_pointer_cast<Integer>(all[0])->getValue();
            int v2 = std::static_pointer_cast<Integer>(all[1])->getValue();
            if(operation=="eq")
                std::make_shared<Boolean>(v1 == v2);
            if(operation=="neq")
                std::make_shared<Boolean>(v1 != v2);
            if(operation=="lt")
                std::make_shared<Boolean>(v1 < v2);
            if(operation=="le")
                std::make_shared<Boolean>(v1 <= v2);
            if(operation=="gt")
                std::make_shared<Boolean>(v1 > v2);
            if(operation=="ge")
                std::make_shared<Boolean>(v1 >= v2);
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
        if(all.size()==1 && all[0]->getType()=="float" && operation=="copy")
            return std::make_shared<Float>(value);
        if(all.size()==2 
            && (all[0]->getType()=="float" || all[0]->getType()=="int") 
            && (all[1]->getType()=="float" || all[1]->getType()=="int")) { 
            float v1 = all[0]->getType()=="int"?std::static_pointer_cast<Integer>(all[0])->getValue():std::static_pointer_cast<Float>(all[0])->getValue();
            float v2 = all[1]->getType()=="int"?std::static_pointer_cast<Integer>(all[1])->getValue():std::static_pointer_cast<Float>(all[1])->getValue();
            if(operation=="eq")
                std::make_shared<Boolean>(v1 == v2);
            if(operation=="neq")
                std::make_shared<Boolean>(v1 != v2);
            if(operation=="lt")
                std::make_shared<Boolean>(v1 < v2);
            if(operation=="le")
                std::make_shared<Boolean>(v1 <= v2);
            if(operation=="gt")
                std::make_shared<Boolean>(v1 > v2);
            if(operation=="ge")
                std::make_shared<Boolean>(v1 >= v2);
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
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()=="code" && operation=="copy")
            return std::make_shared<Code>(start, end, declarationMemory);
        throw Unimplemented();
    }
    std::shared_ptr<Memory>& getDeclarationMemory() {
        return declarationMemory;
    }
};