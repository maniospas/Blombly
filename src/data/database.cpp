#include "data/Database.h"
#include "data/BError.h"
#include "data/BFile.h"
#include "data/BString.h"
#include "data/List.h"
#include "data/BHashMap.h"
#include "common.h"
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;
extern BError* OUT_OF_RANGE;
extern bool isAllowedLocationNoNorm(const std::string& path);
extern bool isAllowedWriteLocationNoNorm(const std::string& path);
extern std::string normalizeFilePath(const std::string& path);
extern void ensureWritePermissionsNoNorm(const std::string& dbPath);


Database::Database(const std::string& dbPath_) : Data(SQLLITE), dbPath(normalizeFilePath(dbPath_)), db(nullptr) {
    if(!dbPath.size() || dbPath[0]==':') {
        int rc = sqlite3_open_v2(dbPath.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
        if (rc != SQLITE_OK) bberror("Failed to open SQLite database: " + std::string(sqlite3_errmsg(db)));
        openDatabase();
        return;
    }

    bbassert(isAllowedLocationNoNorm(dbPath), "Access denied for database path: " + dbPath +
                                        "\n   \033[33m!!!\033[0m Add read permissions using `!access \"location\"`.");

    int flags = isAllowedWriteLocationNoNorm(dbPath) ? (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) : SQLITE_OPEN_READONLY;
    if(flags!=SQLITE_OPEN_READONLY) ensureWritePermissionsNoNorm(dbPath);
    int rc = sqlite3_open_v2(dbPath.c_str(), &db, flags, nullptr);
    if (rc == SQLITE_CANTOPEN) {
        std::string error;
        if(flags==SQLITE_OPEN_READONLY) 
            error = "Write access denied for database path: " + dbPath +
                    "\n   \033[33m!!!\033[0m Add modify permissions using `!modify \"location\"`.";
        else error = "Failed to open SQLite database: " + std::string(sqlite3_errmsg(db));
        bberror(error);
    }
    if (rc != SQLITE_OK) bberror("Failed to open SQLite database: " + std::string(sqlite3_errmsg(db)));
    openDatabase();
}

Database::~Database() {
    if (db) sqlite3_close(db);

}

void Database::openDatabase() {
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) bberror("Failed to open SQLite database: " + std::string(sqlite3_errmsg(db)));
}

void Database::checkModifyPermission() {
    bbassert(isAllowedWriteLocationNoNorm(dbPath), "Write access denied for database path: " + dbPath +
                                             "\n   \033[33m!!!\033[0m Add modify permissions using `!modify \"location\"`.");
}

void Database::clear() {
    bbassert(db, "Database connection has been derminated.");
    if (db) sqlite3_close(db);
}

std::string Database::toString(BMemory* memory) {return dbPath;}

Result Database::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    if (operation == CLEAR && args->size == 1) {
        clear();
        return std::move(Result(nullptr));
    }
    if (operation == TOSTR && args->size == 1) STRING_RESULT(toString(memory));
    if (operation == AT && args->size == 2 && args->arg1->getType() == STRING) {
        std::string query = args->arg1->toString(memory);
        char* errMsg = nullptr;

        // Create a new BList to hold the results
        auto resultList = new BList();

        // Callback function to capture query results
        auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
            BList* resultList = static_cast<BList*>(data);

            // Create a new BHashMap for each row
            auto rowMap = new BHashMap();
            for (int i = 0; i < argc; ++i) {
                // Insert key-value pairs as BString instances
                rowMap->put(new BString(colName[i]), new BString(argv[i] ? argv[i] : "NULL"));
            }
            rowMap->addOwner();
            resultList->contents.push_back(rowMap);
            return 0;
        };

        // Execute the query
        int rc = sqlite3_exec(db, query.c_str(), callback, resultList, &errMsg);
        if (rc == SQLITE_READONLY) {
            std::string error;
            if (!isAllowedWriteLocationNoNorm(dbPath)) {
                error = "Write access denied for database path: " + dbPath +
                        "\n   \033[33m!!!\033[0m This is likely due to missing permissions. Add modify permissions using `!modify \"location\"`.";
            }
            else error = "SQL error: " + std::string(errMsg);
            bberror(error);
        } else if (rc != SQLITE_OK) {
            std::string error = "SQL error: " + std::string(errMsg);
            sqlite3_free(errMsg);
            bberror(error);
        } 
        return std::move(Result(resultList));
    }


    throw Unimplemented();
}

