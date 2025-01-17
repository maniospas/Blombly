#include "common.h"
#include "BMemory.h"
#include <unordered_map>
#include <iostream>


VariableManager variableManager; // .lastId will always be 0 (currently it is disabled)
std::unordered_map<std::string, OperationType> toOperationTypeMap;
void initializeOperationMapping() {
    for (int i = 0; i < sizeof(OperationTypeNames) / sizeof(OperationTypeNames[0]); ++i) {
        // std::cout << OperationTypeNames[i] << " "<<static_cast<OperationType>(i)<<"\n";
        toOperationTypeMap[OperationTypeNames[i]] = static_cast<OperationType>(i);
    }
}

OperationType getOperationType(const std::string &str) {
    auto it = toOperationTypeMap.find(str);
    if (it != toOperationTypeMap.end()) return it->second;
    bberror("Invalid operation name "+str);
    return TOCOPY;
}

std::string getOperationTypeName(OperationType type) {
    return OperationTypeNames[type];
}
