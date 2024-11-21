#ifndef JITABLE_H
#define JITABLE_H

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include "tsl/hopscotch_map.h"
#include "data/Data.h"
#include "data/Code.h"

// Forward declaration of classes
class BMemory;
class Data;
class Code;
class Command;

// Abstract base class representing a jitable code segment
class Jitable {
public:
    virtual ~Jitable() = default;
    virtual bool run(BMemory* memory, Data*& returnValue, bool &returnSignal) = 0;
};

// Class representing a Jitable that returns a primitive
class ReturnPrimitiveJitable : public Jitable {
private:
    Data* primitive;
public:
    explicit ReturnPrimitiveJitable(Data* primitive);
    virtual bool run(BMemory* memory, Data*& returnValue, bool &returnSignal) override;
};

// Function to perform JIT compilation of code
Jitable* jit(const Code* code);

#endif // JITABLE_H
