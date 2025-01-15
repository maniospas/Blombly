#ifndef BFILE_H
#define BFILE_H

#include <string>
#include <memory>
#include <vector>
#include "data/Data.h"
#include <mutex>


class BFile : public Data {
private:
    std::string path;
    int64_t size;
    std::vector<std::string> contents;
    bool contentsLoaded;
    void loadContents();
    bool exists() const;
    std::string username;
    std::string password;
    long timeout = 0;
    mutable std::recursive_mutex memoryLock;
public:
    explicit BFile(const std::string& path_);

    std::string toString(BMemory* memory)override;
    std::string getPath() const;
    virtual Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;

    friend class Boolean;
    friend class Integer;
    friend class BString;
    friend class BList;
};

#endif // BFILE_H
