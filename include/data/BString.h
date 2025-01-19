#ifndef BSTRING_H
#define BSTRING_H

#include <string>
#include <memory>
#include "data/Data.h"
#include <mutex>

class BufferedString {
public:
    std::string value;
    int start;
    int size;
    explicit BufferedString(const std::string& val) : value(val), start(0), size(val.size()) {}
    explicit BufferedString(const std::string& val, int start, int size) : value(val), start(start), size(size) {
        bbassert(start>=0, "Internal error: negative start index at BufferedString");
        bbassert(size<=val.size(), "Internal error: out of bounds at BufferedString");
    }
};


class BString : public Data {
private:
    std::vector<std::shared_ptr<BufferedString>> buffer;
    void consolidate();
    int64_t size;
    explicit BString();
    mutable std::recursive_mutex memoryLock; 

public:
    explicit BString(const std::string& val);
    
    bool isSame(const DataPtr& other) override;
    Result eq(BMemory *memory, const DataPtr& other) override;
    Result neq(BMemory *memory, const DataPtr& other) override;
    Result at(BMemory *memory, const DataPtr& other) override;
    size_t toHash() const override;
    std::string toString(BMemory* memory) override;
    int64_t toInt(BMemory *memory) override;
    int64_t len(BMemory *memory) override;
    double toFloat(BMemory *memory) override;
    bool toBool(BMemory *memory) override;
    Result iter(BMemory *memory) override;
    Result add(BMemory *memory, const DataPtr& other) override;
};

#endif // BSTRING_H
