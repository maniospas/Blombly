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

#include "Result.h"
#include "data/Data.h"

Result::Result(DataPtr data) noexcept : data(data)  {data.existsAddOwner();}
Result::Result(Result& other) noexcept : data(other.data) {data.existsAddOwner();}
Result::Result(Result&& other) noexcept : data(std::move(other.data)) {other.data = DataPtr::NULLP;}
Result::~Result() {data.existsRemoveFromOwner();}
const DataPtr& Result::get() const {return data;}

Result& Result::operator=(const Result& other) {
    if (this != &other) {
        auto prevData = data;
        data = other.data;
        data.existsAddOwner();
        prevData.existsRemoveFromOwner();
    }
    return *this;
}

Result& Result::operator=(Result&& other) noexcept {
    if (this != &other) {
        data.existsRemoveFromOwner();
        data = std::move(other.data);
        other.data = DataPtr::NULLP;
    }
    return *this;
}

