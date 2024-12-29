#ifndef LIST_H
#define LIST_H

#include <memory>
#include <vector>
#include <mutex>
#include "data/Data.h"


class BList : public Data {
private:
    mutable std::recursive_mutex memoryLock; 
    int64_t front;
    void resizeContents();

public:
    std::vector<Data*> contents;
    
    explicit BList();
    explicit BList(int64_t reserve);
    ~BList();

    std::string toString(BMemory* memory)override;
    virtual Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;
    Data* at(int64_t index) const;
};

#endif // LIST_H
