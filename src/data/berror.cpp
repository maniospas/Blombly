// BString.cpp
#include "data/BError.h"
#include "data/BString.h"
#include "common.h"

BError::BError(const std::string& val) : value(val), consumed(false), Data(ERRORTYPE) {}
void BError::consume() {consumed = true;}
bool BError::isConsumed() const {return consumed; }
std::string BError::toString(BMemory* memory){return value;}
