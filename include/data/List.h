// data/List.h
#ifndef LIST_H
#define LIST_H

#include <memory>
#include <vector>
#include "data/Data.h"

// Thread-safe container for list contents
class ListContents {
private:
    pthread_mutex_t memoryLock;

public:
    std::vector<Data*> contents;
    int lockable;

    ListContents();
    ~ListContents();
    void lock();
    void unlock();
    void unsafeUnlock();
};

// List class representing a list of data items
class BList : public Data {
private:

public:
    std::shared_ptr<ListContents> contents;
    explicit BList();
    explicit BList(int reserve);
    explicit BList(const std::shared_ptr<ListContents>& cont);
    ~BList();

    int getType() const override;
    std::string toString() const override;
    Data* shallowCopy() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;
    Data* at(int index) const;
};

#endif // LIST_H
