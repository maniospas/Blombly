#ifndef DATA_H
#define DATA_H

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include "common.h"

class Data;
struct BuiltinArgs {
    Data* arg0;
    Data* arg1;
    Data* arg2;
    uint8_t size;
    //Data* preallocResult;
};

// Abstract base class for all data types
class Data {
public:
    virtual std::string toString() const = 0;
    virtual bool isTrue() const { return false; }
    inline uint8_t getType() const { return type; }
    inline bool isBuiltIn() const {return builtin;}

    Data(int type);
    virtual ~Data();

    static Data* run(const OperationType operation, BuiltinArgs* args);
    virtual Data* implement(const OperationType operation, BuiltinArgs* args);
    virtual size_t toHash() const;
    static int countObjects();
    
    Data* setAsBuiltin() {builtin=this;return this;}
private:
    static int numObjects;
    uint8_t type;
    bool builtin;
};

#endif // DATA_H
