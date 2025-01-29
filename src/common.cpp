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

#include "common.h"
#include "BMemory.h"
#include <unordered_map>
#include <iostream>


VariableManager variableManager; // .lastId will always be 0 (currently it is disabled)
std::unordered_map<std::string, OperationType> toOperationTypeMap;
void initializeOperationMapping() {
    for (int i = 0; i < sizeof(OperationTypeNames) / sizeof(OperationTypeNames[0]); ++i) {
        toOperationTypeMap[OperationTypeNames[i]] = static_cast<OperationType>(i);
    }
}

OperationType getOperationType(const std::string &str) {
    auto it = toOperationTypeMap.find(str);
    if (it != toOperationTypeMap.end()) return it->second;
    bberror("Invalid operation name "+str);
    return END;
}

std::string getOperationTypeName(OperationType type) {return OperationTypeNames[type];}


DataPtr DataPtr::NULLP((Data*)nullptr);