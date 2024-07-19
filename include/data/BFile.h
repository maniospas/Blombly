// data/BString.h
#ifndef BFILE_H
#define BFILE_H

#include <string>
#include <memory>
#include <vector>
#include "data/Data.h"
#include "data/BString.h"


class RawFile {
private:
    pthread_mutex_t memoryLock;

public:
    std::string path;
    int size;
    int lockable; // counts shared instances-1
    std::vector<std::string> contents;

    explicit RawFile(const std::string& path_);

    void lock();
    void unlock();
    void unsafeUnlock(); // used only in Vector's destructor
};


// BFile class representing a non-loaded file
class BFile : public Data {
private:
    std::shared_ptr<RawFile> rawFile;

public:
    explicit BFile(const std::string& path_);
    explicit BFile(const std::shared_ptr<RawFile>& rawFile_);

    int getType() const override;
    std::string toString() const override;
    std::string getPath() const;
    Data* shallowCopy() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;

    friend class Boolean;
    friend class Integer;
    friend class BString;
    friend class BList;
};

#endif // BFILE_H
