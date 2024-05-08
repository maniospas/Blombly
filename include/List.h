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
    std::vector<std::shared_ptr<Data>> contents;

    ListContents();
    void lock();
    void unlock();
};

// List class representing a list of data items
class List : public Data {
private:

public:
    std::shared_ptr<ListContents> contents;
    List();
    explicit List(std::shared_ptr<ListContents> cont);

    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, const BuiltinArgs* args) override;
};

#endif // LIST_H
