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

#ifndef JITABLE_H
#define JITABLE_H

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include "tsl/hopscotch_map.h"
#include "data/Data.h"
#include "data/Code.h"

// Forward declaration of classes
class BMemory;
class Data;
class Code;
class Command;

// Abstract base class representing a jitable code segment
class Jitable {
public:
    virtual ~Jitable() = default;
    virtual bool run(BMemory* memory, DataPtr& returnValue, bool &returnSignal, bool forceStayInThread) = 0;
    virtual bool runWithBooleanIntent(BMemory* memory, bool &returnValue, bool forceStayInThread) {return false;}
    virtual std::string toString() = 0;
};

// Function to perform JIT compilation of code
Jitable* jit(const Code* code);

#endif // JITABLE_H
