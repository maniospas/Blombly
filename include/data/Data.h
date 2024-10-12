#ifndef DATA_H
#define DATA_H

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <atomic>
#include "common.h"

class Data;
class BMemory;
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

    Data(int type);
    virtual ~Data();

    static Data* run(const OperationType operation, BuiltinArgs* args);
    virtual Data* implement(const OperationType operation, BuiltinArgs* args);
    virtual size_t toHash() const;
    static int countObjects();
    
    void addOwner();
    virtual void removeFromOwner();
protected:
    std::atomic<int> referenceCounter;
private:
    static int numObjects;
    uint8_t type;
    bool isContained;
};

#endif // DATA_H
