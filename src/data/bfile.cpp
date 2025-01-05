#include "data/BFile.h"
#include "data/Integer.h"
#include "data/BString.h"
#include "data/Boolean.h"
#include "data/Iterator.h"
#include "data/List.h"
#include "data/BError.h"
#include "common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <curl/curl.h> // Include libcurl
#include <filesystem> // Include filesystem library

namespace fs = std::filesystem;

extern BError* OUT_OF_RANGE;

// Helper function to handle HTTP GET responses
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

// Function to perform HTTP GET request
std::string fetchHttpContent(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string response;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Perform the request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            bberror("Failed to fetch URL: " + url + ", error: " + curl_easy_strerror(res));
        }

        // Cleanup
        curl_easy_cleanup(curl);
    } else {
        bberror("Failed to initialize CURL");
    }
    return response;
}

BFile::BFile(const std::string& path_) : path(path_), size(0), Data(FILETYPE), contentsLoaded(false) {}

void BFile::loadContents() {
    if (contentsLoaded) return;
    if (path.rfind("http", 0) == 0) { // Check if path starts with "http"
        std::string httpContent = fetchHttpContent(path);
        bbassert(!httpContent.empty(), "Failed to fetch content from HTTP path: " + path);
        std::istringstream stream(httpContent);
        std::string line;
        while (std::getline(stream, line)) contents.push_back(line);
    } 
    else if (fs::is_directory(path)) { // Check if path is a directory
        for (const auto& entry : fs::directory_iterator(path)) contents.push_back(entry.path().string());
    } 
    else { // Assume path is a file
        std::ifstream file(path);
        bbassert(file.is_open(), "Failed to open file: "+path);
        std::string line;
        while (std::getline(file, line)) contents.push_back(line);
        file.close();
    }
    size = contents.size();
    contentsLoaded = true;
}

std::string BFile::toString(BMemory* memory) {
    return path;
}

std::string BFile::getPath() const {
    return path;
}

bool BFile::exists() const {
    if (path.rfind("http", 0) == 0) return true; 
    return fs::exists(path);
}

Result BFile::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    if (operation == AT && args->size == 2 && args->arg1->getType() == BB_INT) {
        loadContents();
        int64_t lineNum = static_cast<Integer*>(args->arg1)->getValue();
        if (lineNum < 0 || lineNum >= contents.size()) return std::move(Result(OUT_OF_RANGE));
        std::string lineContent = contents[lineNum];
        STRING_RESULT(lineContent);
    }
    if(operation == DIV && args->size == 2 && args->arg1->getType() == STRING) {
        std::string subpath = args->arg1->toString(memory);
        fs::path basePath(path);
        fs::path combinedPath = basePath / subpath;
        return std::move(Result(new BFile(combinedPath.string())));
    }
    if (operation == TOBB_BOOL && args->size == 1) {
        bool fileExists = exists();
        BB_BOOLEAN_RESULT(fileExists);
    }
    if (operation == PUSH && args->size == 2 && args->arg1->getType() == STRING) {
        std::string newContents = args->arg1->toString(memory);
        fs::path filePath(path);
        if (!fs::exists(filePath.parent_path())) fs::create_directories(filePath.parent_path());
        std::ofstream file(path, std::ios::trunc); 
        bbassert(file.is_open(), "Failed to open file for writing: " + path);
        file << newContents;
        file.close();
        contents.clear();
        std::istringstream stream(newContents);
        std::string line;
        while (std::getline(stream, line)) contents.push_back(line);
        size = contents.size();
        return std::move(Result(nullptr));
    }
    if (args->size == 1) {
        if (operation == CLEAR) {
            bbassert(path.rfind("http", 0)!=0, "Cannot clear HTTP resource (it does not persist): " + path);
            bbassert(fs::exists(path), "Path does not exist: " + path);
            if (fs::is_regular_file(path)) {
                fs::remove(path);
                size = 0;
                contentsLoaded = false;
                return std::move(Result(nullptr));
            } 
            else if (fs::is_directory(path)) {
                bbassert(fs::is_empty(path), "For safety, only empty directories can be cleared but this has contents: " + path);
                fs::remove(path);
                contents.clear();
                size = 0;
                contentsLoaded = false;
                return std::move(Result(nullptr));
            } 
            else bberror("Path is neither a regular file nor a directory: " + path);
        }
        if (operation == LEN) {
            loadContents();
            int64_t ret = contents.size();
            BB_INT_RESULT(ret);
        }
        if (operation == TOFILE) return std::move(Result(this));
        if (operation == TOSTR) STRING_RESULT(toString(memory));
        /*if (operation == POP) {
            loadContents();
            std::string result = "";
            for (std::size_t i = 0; i < contents.size(); ++i) {
                if (i) result += "\n";
                result += contents[i];
            }
            STRING_RESULT(std::move(result));
        }*/
        if (operation == TOITER) return std::move(Result(new AccessIterator(args->arg0)));
        if (operation == TOLIST) {
            loadContents();
            int64_t n = contents.size();
            BList* list = new BList(n);
            for (int64_t i = 0; i < n; ++i) {
                BString* element = new BString(contents[i]);
                element->addOwner();
                element->leak();
                list->contents.push_back(element);
            }
            return std::move(Result(list));
        }
    }
    throw Unimplemented();
}
