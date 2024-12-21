#ifndef BSTRING_H
#define BSTRING_H

#include <string>
#include <memory>
#include "data/Data.h"

class BufferedString {
public:
    std::string value;

    explicit BufferedString(const std::string& val) : value(val) {}
};


class BString : public Data {
private:
    std::vector<std::shared_ptr<BufferedString>> buffer;
    void consolidate();
    int size;
    explicit BString();

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
