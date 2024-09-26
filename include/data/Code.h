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
    tsl::hopscotch_map<int, std::shared_ptr<Data>> metadata;
    Metadata();
    ~Metadata();
};

// Code class representing a segment of code in a program
class Code : public Data {
private:
    int start, end;
    std::weak_ptr<BMemory> declarationMemory;
    std::shared_ptr<Metadata> metadata;
    std::shared_ptr<std::vector<Command*>> program;

public:
    bool scheduleForParallelExecution;
    
    explicit Code(const std::shared_ptr<std::vector<Command*>>& programAt, int startAt, int endAt, const std::shared_ptr<BMemory>& declMemory);
    explicit Code(const std::shared_ptr<std::vector<Command*>>& programAt, int startAt, int endAt, const std::shared_ptr<BMemory>& declMemory, const std::shared_ptr<Metadata>& metadata);
    explicit Code(const std::shared_ptr<std::vector<Command*>>& programAt, int startAt, int endAt, const std::weak_ptr<BMemory>& declMemory);
    explicit Code(const std::shared_ptr<std::vector<Command*>>& programAt, int startAt, int endAt, const std::weak_ptr<BMemory>& declMemory, const std::shared_ptr<Metadata>& metadata);
    
    int getType() const override;
    std::string toString() const override;
    int getStart() const;
    int getEnd() const;
    std::shared_ptr<std::vector<Command*>> getProgram() const;
    
    void setMetadata(int pos, const std::shared_ptr<Data>& data);
    bool getMetadataBool(int id, bool def) const;
    void setDeclarationMemory(const std::shared_ptr<BMemory>& newMemory);

    std::shared_ptr<Data> getMetadata(int id) const;
    std::shared_ptr<BMemory> getDeclarationMemory() const;
    std::shared_ptr<Metadata> getAllMetadata() const;

    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // CODE_H
