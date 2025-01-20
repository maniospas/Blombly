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

    Result iter(BMemory* memory) override;
    void clear(BMemory* memory) override;
    Result push(BMemory* memory, const DataPtr& other) override;
    Result div(BMemory* memory, const DataPtr& other) override;
    int64_t len(BMemory* memory) override;
    Result put(BMemory* memory, const DataPtr& position, const DataPtr& value) override;
    Result at(BMemory* memory, const DataPtr& position) override;
    bool toBool(BMemory* memory) override;
};

#endif // BFILE_H
