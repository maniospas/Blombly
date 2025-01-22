#include "data/BString.h"
#include "data/BFile.h"
#include "data/BError.h"
#include "data/Iterator.h"
#include "data/Database.h"
#include "data/Graphics.h"
#include "common.h"
#include <openssl/evp.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

extern BError* OUT_OF_RANGE;

inline std::string calculateHash(const std::string& input, const EVP_MD* (*hash_function)()) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    bbassert(context, "OpenSSL error: Failed to create EVP_MD_CTX");
    unsigned char hash_result[EVP_MAX_MD_SIZE];
    unsigned int hash_length = 0;
    if (EVP_DigestInit_ex(context, hash_function(), nullptr) != 1 ||
        EVP_DigestUpdate(context, input.c_str(), input.size()) != 1 ||
        EVP_DigestFinal_ex(context, hash_result, &hash_length) != 1) {
        EVP_MD_CTX_free(context);
        bberror("OpenSSL error: Failed to compute hash");
    }
    EVP_MD_CTX_free(context);
    std::ostringstream hex_stream;
    for (unsigned int i = 0; i < hash_length; ++i) hex_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash_result[i]);
    return hex_stream.str();
}

void BString::consolidate() {
    if (!(buffer.size() != 1 || buffer.front()->value.size()!=buffer.front()->size)) return;
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


BString::BString(const std::string& val) : Data(STRING), size(val.size()) {buffer.push_back(std::make_shared<BufferedString>(val));}
BString::BString() : Data(STRING), size(0) {}
size_t BString::toHash() const {return std::hash<std::string>{}(buffer.front()->value);}

std::string BString::toString(BMemory* memory){
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    consolidate();
    return buffer.front()->value;
}

bool BString::isSame(const DataPtr& other) {
    if (other.existsAndTypeEquals(STRING)) return toString(nullptr) == static_cast<BString*>(other.get())->toString(nullptr);
    return false;
}

Result BString::eq(BMemory *memory, const DataPtr& other) {return RESMOVE(Result(isSame(other)));}
Result BString::neq(BMemory *memory, const DataPtr& other) {return RESMOVE(Result(!isSame(other)));}

Result BString::at(BMemory *memory, const DataPtr& other) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if(other.isint()) {
        int64_t index = other.unsafe_toint();
        if (index>=buffer.front()->value.size()) consolidate();
        if (index < 0 || index >= toString(memory).size()) return RESMOVE(Result(OUT_OF_RANGE));
        return RESMOVE(Result(new BString(std::string(1, toString(memory)[index]))));
    }
    if(other.existsAndTypeEquals(STRING)) {
        std::string v1 = toString(nullptr);
        std::string v2 = static_cast<BString*>(other.get())->toString(nullptr);

        bbassert(v2 == "md5" || v2 == "sha1" || v2 == "sha256" || v2 == "sha512" ||
                    v2 == "sha224" || v2 == "sha384" || v2 == "sha3_224" || v2 == "sha3_256" || 
                    v2 == "sha3_384" || v2 == "sha3_512" || v2 == "blake2b" || v2 == "blake2s" || 
                    v2 == "ripemd160" || v2 == "whirlpool" || v2 == "sm3", 
                    "Only md5, sha1, sha224, sha256, sha384, sha512, sha3_224, sha3_256, sha3_384, sha3_512, blake2b, blake2s, ripemd160, whirlpool, or sm3 formatting is allowed for strings");
            
        if (v2 == "sha1") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha1))));
        if (v2 == "sha224") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha224))));
        if (v2 == "sha256") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha256))));
        if (v2 == "sha384") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha384))));
        if (v2 == "sha512") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha512))));
        if (v2 == "sha3_224") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha3_224))));
        if (v2 == "sha3_256") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha3_256))));
        if (v2 == "sha3_384") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha3_384))));
        if (v2 == "sha3_512") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha3_512))));
        if (v2 == "blake2b") return RESMOVE(Result(new BString(calculateHash(v1, EVP_blake2b512))));
        if (v2 == "blake2s") return RESMOVE(Result(new BString(calculateHash(v1, EVP_blake2s256))));
        if (v2 == "ripemd160") return RESMOVE(Result(new BString(calculateHash(v1, EVP_ripemd160))));
        if (v2 == "whirlpool") return RESMOVE(Result(new BString(calculateHash(v1, EVP_whirlpool))));
        if (v2 == "sm3") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sm3))));
        
        return RESMOVE(Result(new BString(calculateHash(v1, EVP_md5))));
    }

    if(other.exists()) {
        auto res = other->iter(memory);
        DataPtr _iterator = res.get();
        bbassert(_iterator.existsAndTypeEquals(ITERATOR), "String index is neither an integer nor can be converted to an iterator viat `iter`: "+other->toString(memory));
        Iterator* iterator = static_cast<Iterator*>(_iterator.get());

        // Treat contiguous iterators more efficiently
        if (iterator->isContiguous()) {
            int64_t start = iterator->getStart();
            int64_t end = iterator->getEnd();
            if (start < 0 || start >= toString(memory).size() || end < 0 || end > toString(memory).size() || start > end) return RESMOVE(Result(OUT_OF_RANGE));
            std::string result = toString(memory).substr(start, end - start);
            return RESMOVE(Result(new BString(std::move(result))));
        } else {
            // Handle non-contiguous iterators
            std::string result;
            result.reserve(iterator->expectedSize());
            auto front = buffer.front();
            while (true) {
                Result nextResult = iterator->next(memory);
                DataPtr indexData = nextResult.get();
                if (indexData == OUT_OF_RANGE) break; 
                bbassert(indexData.isint(), "String index iterator must contain integers: "+other->toString(memory));
                int64_t index = indexData.unsafe_toint();
                if (index < 0 || index >= size) return RESMOVE(Result(OUT_OF_RANGE));
                result += front->value[index];
            }
            return RESMOVE(Result(new BString(result)));
        }
    }

    bberror("Strings only accept integer, string, or iterable indexes.");
}

int64_t BString::toInt(BMemory *memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    consolidate();
    const std::string& v1 = buffer.front()->value;
    char* endptr = nullptr;
    int64_t ret = std::strtol(v1.c_str(), &endptr, 10);
    if (endptr == v1.c_str() || *endptr != '\0') bberror("Failed to convert string to int");
    return ret;
}

double BString::toFloat(BMemory *memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    consolidate();
    const std::string& v1 = buffer.front()->value;
    char* endptr = nullptr;
    double ret = std::strtod(toString(memory).c_str(), &endptr);
    if (endptr == toString(memory).c_str() || *endptr != '\0') bberror("Failed to convert string to float");
    return ret;
}

bool BString::toBool(BMemory *memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    consolidate();
    const std::string& v1 = buffer.front()->value;
    if(v1=="true") return true;
    if(v1=="false") return false;
    bberror("Failed to convert string to bool");
}

Result BString::iter(BMemory *memory) {return RESMOVE(Result(new AccessIterator(this, size)));}
Result BString::add(BMemory *memory, const DataPtr& other) {
    bbassert(other.existsAndTypeEquals(STRING), "Strings can only be concatenated with other strings");
    return RESMOVE(Result(new BString(toString(nullptr)+static_cast<BString*>(other.get())->toString(nullptr))));
}
int64_t BString::len(BMemory *memory) {return size;}