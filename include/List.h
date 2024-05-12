// List.h
#ifndef LIST_H
#define LIST_H

#include <memory>
#include <vector>
#include "Data.h"

// Thread-safe container for list contents
class ListContents {
private:
    pthread_mutex_t memoryLock;

public:
    std::vector<Data*> contents;

    ListContents();
    ~ListContents();
    void lock();
    void unlock();
};

// List class representing a list of data items
class BList : public Data {
private:

public:
    std::shared_ptr<ListContents> contents;
    explicit BList();
    explicit BList(int reserve);
    explicit BList(const std::shared_ptr<ListContents>& cont);

    int getType() const override;
    std::string toString() const override;
    Data* shallowCopy() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // LIST_H
