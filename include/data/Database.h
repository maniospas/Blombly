/*
   Copyright 2024 Emmanouil Krasanakis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

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
