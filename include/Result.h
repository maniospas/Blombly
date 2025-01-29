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

#ifndef RESULT_H
#define RESULT_H

#include "common.h"

class Result {
private:
    DataPtr data;

public:
    explicit Result(DataPtr data) noexcept;
    //explicit Result(const DataPtr& data) noexcept;
    explicit Result(Result& other) noexcept;
    Result(Result&& other) noexcept;
    ~Result();
    Result& operator=(const Result& other);
    Result& operator=(Result&& other) noexcept;
    const DataPtr& get() const;
};

#endif // RESULT_H
