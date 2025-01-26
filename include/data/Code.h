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
    size_t start, end, premature_end;
    const std::vector<Command>* program;

public:
    bool scheduleForParallelExecution;
    Jitable* jitable;
    
    explicit Code(const std::vector<Command>* programAt, size_t startAt, size_t endAt, size_t premature_end);
    Code* copy() const {Code* ret = new Code(program, start, end, premature_end);ret->jitable=jitable; return ret;}
    std::string toString(BMemory* memory)override;
    size_t getStart() const;
    size_t getEnd() const;
    size_t getOptimizedEnd() const;
    const std::vector<Command>* getProgram() const;
};

#endif // CODE_H
