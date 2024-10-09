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
    int size;
    Data* preallocResult;
};

// Abstract base class for all data types
class Data : public std::enable_shared_from_this<Data> {
public:
    virtual std::string toString() const = 0;
    virtual bool isTrue() const { return false; }
    inline int getType() const { return type; }

    Data(int type);
    virtual ~Data();

    static Data* run(const OperationType operation, BuiltinArgs* args);
    virtual Data* implement(const OperationType operation, BuiltinArgs* args);
    virtual size_t toHash() const;
    static int countObjects();

private:
    static int numObjects;
    int type;
};

#endif // DATA_H
