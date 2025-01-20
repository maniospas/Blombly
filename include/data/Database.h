#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <sqlite3.h>
#include "data/Data.h"
#include "data/BFile.h"

class Database : public Data {
private:
    std::string dbPath;
    sqlite3* db;
    void openDatabase();
    void checkModifyPermission();

public:
    explicit Database(const std::string& dbPath_);
    ~Database();

    void clear();
    std::string toString(BMemory* memory) override;
    void clear(BMemory* memory) override;
    Result push(BMemory* memory, const DataPtr& other) override;

};

#endif // DATABASE_H
