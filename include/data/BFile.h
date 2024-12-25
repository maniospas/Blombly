#ifndef BFILE_H
#define BFILE_H

#include <string>
#include <memory>
#include <vector>
#include "data/Data.h"


class BFile : public Data {
private:
    std::string path;
    int size;
    std::vector<std::string> contents;
    bool contentsLoaded;
    void loadContents();
    bool exists() const;
public:
    explicit BFile(const std::string& path_);

    std::string toString()override;
    std::string getPath() const;
    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;

    friend class Boolean;
    friend class Integer;
    friend class BString;
    friend class BList;
};

#endif // BFILE_H
