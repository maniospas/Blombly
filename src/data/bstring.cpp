#include "data/BString.h"
#include "data/Boolean.h"
#include "data/Integer.h"
#include "data/BFloat.h"
#include "data/BFile.h"
#include "data/BError.h"
#include "data/Iterator.h"
#include "common.h"

extern BError* OUT_OF_RANGE;

void BString::consolidate() {
    if (buffer.size() != 1 || buffer.front()->value.size()!=buffer.front()->size) {
        // Reserve a single contiguous string for the entire content.
        std::string merged;
        merged.reserve(size);

        for (const auto& part : buffer) {
            if(part->size==part->value.size()) {
                merged += part->value;
                continue;
            }
            const char* start_ptr = part->value.data() + part->start; // Direct pointer access
            merged.append(start_ptr, part->size); // Efficiently append without intermediate substr
        }

        buffer.clear();
        buffer.push_back(std::make_shared<BufferedString>(std::move(merged)));
    }
}



BString::BString(const std::string& val) : Data(STRING), size(val.size()) {
    buffer.push_back(std::make_shared<BufferedString>(val));
}

BString::BString() : Data(STRING), size(0) {
}

std::string BString::toString(BMemory* memory){
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    consolidate();
    return buffer.front()->value;
}

bool BString::isSame(Data* other) {
    if (other->getType() != STRING) 
        return false;
    return toString(nullptr) == static_cast<BString*>(other)->toString(nullptr);
}

size_t BString::toHash() const {
    return std::hash<std::string>{}(buffer.front()->value);
}

Result BString::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if((operation!=ADD && operation!=LEN && operation!=AT && operation!=TOITER) || buffer.size()>512)
        consolidate();
    if (args->size == 2 && args->arg0->getType() == STRING && args->arg1->getType() == STRING) {
        if(operation==ADD) {
            auto v1 = static_cast<BString*>(args->arg0);
            auto v2 = static_cast<BString*>(args->arg1);
            std::lock_guard<std::recursive_mutex> lock(v2->memoryLock);
            BString* ret = new BString();
            ret->buffer.insert(ret->buffer.end(), v1->buffer.begin(), v1->buffer.end());
            ret->buffer.insert(ret->buffer.end(), v2->buffer.begin(), v2->buffer.end());
            ret->size = v1->size + v2->size;
            return std::move(Result(ret)); 
        }
        std::string v1 = static_cast<BString*>(args->arg0)->toString(memory);
        std::string v2 = static_cast<BString*>(args->arg1)->toString(memory);
        switch (operation) {
            case EQ: BB_BOOLEAN_RESULT(v1 == v2);
            case NEQ: BB_BOOLEAN_RESULT(v1 != v2);
            case ADD: STRING_RESULT(v1+v2);
        }
        throw Unimplemented();
    }

    if (args->size == 1) {
        switch (operation) {
            case TOCOPY:
            case TOSTR: STRING_RESULT(toString(memory));
            case TOBB_INT: {
                char* endptr = nullptr;
                int64_t ret = std::strtol(toString(memory).c_str(), &endptr, 10);
                if (endptr == toString(memory).c_str() || *endptr != '\0') {
                    return std::move(Result(new BError("Failed to convert string to int")));
                }
                BB_INT_RESULT(ret);
            }
            case LEN: BB_INT_RESULT(size);
            case TOBB_FLOAT: {
                char* endptr = nullptr;
                double ret = std::strtod(toString(memory).c_str(), &endptr);
                if (endptr == toString(memory).c_str() || *endptr != '\0') {
                    return std::move(Result(new BError("Failed to convert string to float")));
                }
                BB_FLOAT_RESULT(ret);
            }
            case TOBB_BOOL: BB_BOOLEAN_RESULT(toString(memory) == "true");
            case TOITER: return std::move(Result(new AccessIterator(args->arg0)));
            case TOFILE: return std::move(Result(new BFile(toString(memory))));
        }
        throw Unimplemented();
    }

    if (operation == AT && args->size == 2 && args->arg1->getType() == BB_INT) {
        int64_t index = static_cast<Integer*>(args->arg1)->getValue();
        if(index>=buffer.front()->value.size())
            consolidate();
        if (index < 0 || index >= toString(memory).size()) {
            return std::move(Result(OUT_OF_RANGE));
        }
        return std::move(Result(new BString(std::string(1, toString(memory)[index]))));
    }

    if (operation == AT && args->size == 2 && (args->arg1->getType()==STRUCT || args->arg1->getType()==LIST || args->arg1->getType()==ITERATOR)) {
        // Obtain an iterator from the object
        BuiltinArgs implargs;
        implargs.size = 1;
        implargs.arg0 = args->arg1;
        auto res = args->arg1->implement(TOITER, &implargs, memory);
        Data* _iterator = res.get();
        bbassert(_iterator && _iterator->getType() == ITERATOR, "String index is neither an integer nor can be converted to an iterator viat `iter`: "+args->arg1->toString(memory));
        Iterator* iterator = static_cast<Iterator*>(_iterator);

        // Treat contiguous iterators more efficiently
        if (iterator->isContiguous()) {
            int64_t start = iterator->getStart();
            int64_t end = iterator->getEnd();
            if (start < 0 || start >= toString(memory).size() || end < 0 || end > toString(memory).size() || start > end) 
                return std::move(Result(OUT_OF_RANGE));
            std::string result = toString(memory).substr(start, end - start);
            return std::move(Result(new BString(std::move(result))));
        } else {
            // Handle non-contiguous iterators
            std::string result;
            result.reserve(iterator->expectedSize());
            auto front = buffer.front();
            BuiltinArgs nextArgs;
            nextArgs.arg0 = iterator;
            while (true) {
                nextArgs.size = 1;  // important to have this here, as the iterator adds an argument to nextArgs internally to save up on memory
                Result nextResult = iterator->implement(NEXT, &nextArgs, memory);
                Data* indexData = nextResult.get();
                if (indexData == OUT_OF_RANGE) 
                    break; 
                bbassert(indexData && indexData->getType() == BB_INT, "String index iterator must contain integers: "+args->arg1->toString(memory));
                int index = static_cast<Integer*>(indexData)->getValue();
                if (index < 0 || index >= size) 
                    return std::move(Result(OUT_OF_RANGE));
                result += front->value[index];
            }

            return std::move(Result(new BString(result)));
        }
    }



    throw Unimplemented();
}
