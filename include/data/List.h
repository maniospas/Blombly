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
    std::shared_ptr<std::vector<std::shared_ptr<Data>>> contents;
    
    explicit BList();
    explicit BList(int reserve);
    explicit BList(const std::shared_ptr<BList>& list);
    explicit BList(const std::shared_ptr<std::vector<std::shared_ptr<Data>>>& contents);
    ~BList();

    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;
    std::shared_ptr<Data> at(int index) const;
};

#endif // LIST_H
