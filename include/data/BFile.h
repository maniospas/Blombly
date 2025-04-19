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

#ifndef BFILE_H
#define BFILE_H

#include <string>
#include <memory>
#include <vector>
#include <deque>
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
public:
    mutable std::recursive_mutex memoryLock;
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
