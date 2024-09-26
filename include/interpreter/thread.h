#ifndef THREAD_EXECUTE_H
#define THREAD_EXECUTE_H

#include "common.h"
#include "data/Code.h"
#include "data/Struct.h"
#include "BMemory.h"
#include "interpreter/Command.h"
class ThreadResult;


void threadExecute(std::shared_ptr<Code> code,
                   std::shared_ptr<BMemory> memory,
                   std::shared_ptr<ThreadResult> result,
                   const Command& command);

#endif // THREAD_EXECUTE_H