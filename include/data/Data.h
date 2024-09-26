#ifndef DATA_H
#define DATA_H

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include "common.h"

class Data;
struct BuiltinArgs {
    std::shared_ptr<Data> arg0;
    std::shared_ptr<Data> arg1;
    std::shared_ptr<Data> arg2;
    int size;
    std::shared_ptr<Data> preallocResult;
};

// Abstract base class for all data types
class Data : public std::enable_shared_from_this<Data> {
public:
    bool isDestroyable = false;
    
    virtual std::string toString() const = 0;
    virtual int getType() const = 0;
    virtual std::shared_ptr<Data> shallowCopy() const = 0;
    virtual bool isTrue() const { return false; }

    Data();
    virtual ~Data();

    static std::shared_ptr<Data> run(const OperationType operation, BuiltinArgs* args);
    virtual std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args);
    virtual size_t toHash() const;
    static int countObjects();
    
private:
    static int numObjects;
};

#endif // DATA_H
