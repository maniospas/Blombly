#ifndef THREAD_EXECUTE_H
#define THREAD_EXECUTE_H

#include "common.h"
#include "data/Code.h"
#include "data/Struct.h"
#include "BMemory.h"
#include "interpreter/Command.h"
class ThreadResult;


void threadExecute(Code* code, BMemory* memory, ThreadResult* result, Command* command);

#endif // THREAD_EXECUTE_H