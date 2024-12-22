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
    int size;
    explicit BString();
    mutable std::recursive_mutex memoryLock; 

public:
    explicit BString(const std::string& val);

    std::string toString()override;
    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
    virtual bool isSame(Data* other) override;
    virtual size_t toHash() const override;

    friend class Boolean;
    friend class BFloat;
    friend class Integer;
    friend class Vector;
    friend class BList;
    friend class BFile;
    friend class BError;
    friend class BHashMap;
};

#endif // BSTRING_H
