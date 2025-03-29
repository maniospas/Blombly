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


void bberrorexplain(const std::string& msg, const std::string& explanation, const std::string& postfix) {
    std::string ret = "\033[0m(\x1B[31m ERROR \033[0m) " + msg;
    if(explanation.size()) {
        int line_length = 0;
        std::string word;
        std::istringstream iss(explanation);
        ret += "\n  \033[33m!!!\033[0m ";
        while (iss >> word) { 
            if(word=="~") {
                line_length = 0;
                ret += "\n      ";
                continue;
            }
            int word_length = word.size();
            if(line_length+word_length>72) {
                line_length = 0;
                ret += "\n      ";
            }
            ret += word+" ";
            line_length += word_length+1;
        }
    }
    if(postfix.size())
        ret += "\n"+postfix;
    throw BBError(ret);
}


VariableManager variableManager; // .lastId will always be 0 (currently it is disabled)
std::unordered_map<std::string, OperationType> toOperationTypeMap;
void initializeOperationMapping() {
    for (int i = 0; i < sizeof(OperationTypeNames) / sizeof(OperationTypeNames[0]); ++i) toOperationTypeMap[OperationTypeNames[i]] = static_cast<OperationType>(i);
}

OperationType getOperationType(const std::string &str) {
    auto it = toOperationTypeMap.find(str);
    if (it != toOperationTypeMap.end()) return it->second;
    bberror("Invalid operation name "+str);
    return END;
}

std::string getOperationTypeName(OperationType type) {return OperationTypeNames[type];}
DataPtr DataPtr::NULLP((Data*)nullptr);