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
class Jitable;

class Code : public Data {
private:
    int start, end, premature_end;
    const std::vector<Command>* program;

public:
    bool scheduleForParallelExecution;
    Jitable* jitable;
    
    explicit Code(const std::vector<Command>* programAt, int startAt, int endAt, int premature_end);
    
    Code* copy() const {Code* ret = new Code(program, start, end, premature_end);ret->jitable=jitable; return ret;}
    std::string toString(BMemory* memory)override;
    int getStart() const;
    int getEnd() const;
    int getOptimizedEnd() const;
    const std::vector<Command>* getProgram() const;
    virtual Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;
};

#endif // CODE_H
