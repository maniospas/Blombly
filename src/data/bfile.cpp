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
        if (!httpContent.empty()) {
            std::istringstream stream(httpContent);
            std::string line;
            while (std::getline(stream, line)) {
                contents.push_back(line);
            }
        } else {
            bberror("Failed to fetch content from HTTP path: " + path);
        }
    } else if (fs::is_directory(path)) { // Check if path is a directory
        for (const auto& entry : fs::directory_iterator(path)) {
            contents.push_back(entry.path().string());
        }
    } else { // Assume path is a file
        std::ifstream file(path);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                contents.push_back(line);
            }
            file.close();
        } else {
            bberror("Failed to open file: " + path);
        }
    }
    size = contents.size();
    contentsLoaded = true;
}

std::string BFile::toString() {
    loadContents();
    std::string result = "";
    for (std::size_t i = 1; i < contents.size(); ++i) {
        if (i)
            result += "\n";
        result += contents[i];
    }
    return result;
}

std::string BFile::getPath() const {
    return path;
}

bool BFile::exists() const {
    if (path.rfind("http", 0) == 0) return true; 
    return fs::exists(path);
}

Result BFile::implement(const OperationType operation, BuiltinArgs* args) {

    if (operation == AT && args->size == 2 && args->arg1->getType() == BB_INT) {
        loadContents();
        int lineNum = static_cast<Integer*>(args->arg1)->getValue();
        if (lineNum < 0 || lineNum >= contents.size()) {
            return std::move(Result(OUT_OF_RANGE));
        }
        std::string lineContent = contents[lineNum];
        STRING_RESULT(lineContent);
    }
    if(operation == DIV && args->size == 2 && args->arg1->getType() == STRING) {
        std::string subpath = args->arg1->toString();
        fs::path basePath(path);
        fs::path combinedPath = basePath / subpath;
        return std::move(Result(new BFile(combinedPath.string())));
    }
    if (operation == TOBB_BOOL && args->size == 1) {
        bool fileExists = exists();
        BB_BOOLEAN_RESULT(fileExists);
    }
    if (args->size == 1) {
        if (operation == LEN) {
            loadContents();
            int ret = contents.size();
            BB_INT_RESULT(ret);
        }
        if (operation == TOFILE) {
            return std::move(Result(this));
        }
        if (operation == TOSTR) {
            loadContents();
            STRING_RESULT(toString());
        }
        if (operation == TOITER) {
            return std::move(Result(new AccessIterator(args->arg0)));
        }
        if (operation == TOLIST) {
            loadContents();
            int n = contents.size();
            BList* list = new BList(n);
            for (int i = 0; i < n; ++i) {
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
