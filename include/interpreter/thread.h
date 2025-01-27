#ifndef THREAD_EXECUTE_H
#define THREAD_EXECUTE_H

#include "common.h"
#include "data/Code.h"
#include "data/Struct.h"
#include "BMemory.h"
#include "interpreter/Command.h"
class ThreadResult;


void threadExecute(unsigned int depth, Code* code, BMemory* memory, ThreadResult* result, const Command* command, DataPtr thisObj);

#endif // THREAD_EXECUTE_H