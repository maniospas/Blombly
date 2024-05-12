// Code.h
#ifndef CODE_H
#define CODE_H

#include <memory>
#include <string>
#include "Data.h"

// Forward declaration for Memory class
class BMemory;

// Code class representing a segment of code in a program
class Code : public Data {
private:
    int start, end;
    std::shared_ptr<BMemory> declarationMemory;
    void* program;

public:
    Code(void* programAt, int startAt, int endAt, const std::shared_ptr<BMemory>& declMemory);

    int getType() const override;
    std::string toString() const override;
    int getStart() const;
    int getEnd() const;
    void* getProgram() const;
    std::shared_ptr<BMemory> getDeclarationMemory() const;

    Data* shallowCopy() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // CODE_H
