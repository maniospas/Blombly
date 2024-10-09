#ifndef LIST_H
#define LIST_H

#include <memory>
#include <vector>
#include <mutex>
#include "data/Data.h"


class BList : public Data {
private:
    mutable std::recursive_mutex memoryLock; 

public:
    std::vector<Data*> contents;
    
    explicit BList();
    explicit BList(int reserve);
    ~BList();

    int getType() const override;
    std::string toString() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;
    Data* at(int index) const;
};

#endif // LIST_H
