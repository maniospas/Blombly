#ifndef CODE_H
#define CODE_H

#include <memory>
#include <string>
#include <vector>
#include "data/Data.h"
#include "tsl/hopscotch_map.h"

// Forward declaration for Memory and Command classes
class BMemory;
class Command;

class Metadata {
public:
    tsl::hopscotch_map<int, Data*> metadata;
    Metadata();
    ~Metadata();
};

// Code class representing a segment of code in a program
class Code : public Data {
private:
    int start, end;
    BMemory* declarationMemory;
    Metadata* metadata;
    std::vector<Command*>* program;

public:
    bool scheduleForParallelExecution;
    
    explicit Code(std::vector<Command*>* programAt, int startAt, int endAt, BMemory* declMemory);
    explicit Code(std::vector<Command*>* programAt, int startAt, int endAt, BMemory* declMemory, Metadata* metadata);
    
    std::string toString() const override;
    int getStart() const;
    int getEnd() const;
    std::vector<Command*>* getProgram() const;
    
    void setMetadata(int pos, Data* data);
    bool getMetadataBool(int id, bool def) const;
    void setDeclarationMemory(BMemory* newMemory);

    Data* getMetadata(int id) const;
    BMemory* getDeclarationMemory() const;
    Metadata* getAllMetadata() const;

    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // CODE_H
