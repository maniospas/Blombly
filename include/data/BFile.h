#ifndef BFILE_H
#define BFILE_H

#include <string>
#include <memory>
#include <vector>
#include "data/Data.h"
#include <pthread.h>

class BFile : public Data {
private:
    pthread_mutex_t memoryLock;
    std::string path;
    int size;
    int lockable; // counts shared instances - 1
    std::vector<std::string> contents;

public:
    explicit BFile(const std::string& path_);

    int getType() const override;
    std::string toString() const override;
    std::string getPath() const;
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;

    friend class Boolean;
    friend class Integer;
    friend class BString;
    friend class BList;
};

#endif // BFILE_H
