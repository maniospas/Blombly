#include "data/BFile.h"
#include "data/Integer.h"
#include "data/BString.h"
#include "data/Iterator.h"
#include "data/List.h"
#include "data/BError.h"
#include "common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <curl/curl.h> // Include libcurl
#include <algorithm>  // For std::remove
#include <filesystem>
#include <string>
#include <unordered_map>
#include <stdexcept>

namespace fs = std::filesystem;

extern BError* OUT_OF_RANGE;

static std::vector<std::string> allowedLocations = {};
static std::vector<std::string> allowedWriteLocations = {};
extern std::string blombly_executable_path;
std::unordered_map<std::string, std::string> virtualFileSystem;
std::recursive_mutex virtualFileSystemLock;

namespace fs = std::filesystem;

std::string normalizeFilePath(const std::string& path) {
    if(path=="") return "";
    static const std::unordered_map<std::string, fs::path> prefixToBaseDir = {
        //{".//", fs::current_path()},           // Current working directory for terminal paths
        {"bb://", blombly_executable_path},       // Base directory where ./blombly resides
        {"raw://", ""}                           // Direct file path without a specific base
    };
    if (path.find("http://", 0) == 0 
        || path.find("https://", 0) == 0 
        || path.find("ftp://", 0) == 0 
        || path.find("sftp://", 0) == 0 
        || path.find("ftps://", 0) == 0 
        || path.find("vfs://", 0) == 0) return path;

    for (const auto& [prefix, baseDir] : prefixToBaseDir) {
        if (path.find(prefix, 0) == 0) {
            fs::path execFilePath = baseDir / path.substr(prefix.size());
            return fs::weakly_canonical(execFilePath).string();
        }
    }
    return fs::weakly_canonical(fs::current_path()/path).string();
    //bberror("Provided file path `" + path + "` must start with a valid URI prefix (e.g., `http://`, `https://`, or `exec://`, `cwd://`, `file://`)");
}


bool isAllowedLocation(const std::string& path_) {
    std::string path = normalizeFilePath(path_);
    if(path.find("http://", 0)==0 
        || path.find("https://", 0)==0 
        || path.find("ftp://", 0)==0 
        || path.find("sftp://", 0)==0
        || path.find("ftps://", 0)==0
        || path.find("vfs://", 0)==0){}
    else
        path = fs::weakly_canonical(path).string();
    for (const auto& location : allowedLocations) if(path.size() >= location.size() && path.compare(0, location.size(), location) == 0) return true;
    return false;
}

bool isAllowedLocationNoNorm(const std::string& path_) {
    std::string path = path_;
    for (const auto& location : allowedLocations) if(path.size() >= location.size() && path.compare(0, location.size(), location) == 0) return true;
    return false;
}

bool isAllowedWriteLocation(const std::string& path_) {
    std::string path = normalizeFilePath(path_);
    if(path.find("http://", 0)==0 
        || path.find("https://", 0)==0 
        || path.find("ftp://", 0)==0 
        || path.find("sftp://", 0)==0
        || path.find("ftps://", 0)==0
        || path.find("vfs://", 0)==0){}
    else
        path = fs::weakly_canonical(path).string();
    for (const auto& location : allowedWriteLocations) if(path.size() >= location.size() && path.compare(0, location.size(), location) == 0) return true;
    return false;
}
bool isAllowedWriteLocationNoNorm(const std::string& path_) {
    std::string path = path_;
    for (const auto& location : allowedWriteLocations) if(path.size() >= location.size() && path.compare(0, location.size(), location) == 0) return true;
    return false;
}
void addAllowedLocation(const std::string& location_) {
    std::string location = normalizeFilePath(location_);
    if (!isAllowedLocationNoNorm(location)) allowedLocations.push_back(location);
}

void addAllowedWriteLocation(const std::string& location_) {
    std::string location = normalizeFilePath(location_);
    if (!isAllowedWriteLocationNoNorm(location)) allowedWriteLocations.push_back(location);
    addAllowedLocation(location_);
}

void ensureWritePermissionsNoNorm(const std::string& dbPath) {
    namespace fs = std::filesystem;
    
    fs::path filePath(dbPath);
    fs::path parentPath = filePath.parent_path();
    while (!parentPath.empty()) {
        if (fs::exists(parentPath)) {
            if (filePath.parent_path().string().size() && !fs::exists(filePath.parent_path())) fs::create_directories(filePath.parent_path());
            return; 
        }
        parentPath = parentPath.parent_path();
    }
    bberror("There are no parent paths with write access for: " + dbPath + 
            "\n   \033[33m!!!\033[0m Add write permissions using `!access \"location\"`.");
}


void clearAllowedLocations() {
    allowedLocations.clear();
    allowedWriteLocations.clear();
    addAllowedLocation("bb://libs/");
    addAllowedLocation(".");
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    if (response == nullptr) return 0;
    size_t totalSize = size * nmemb;
    try {response->append(static_cast<char*>(contents), totalSize); } 
    catch (const std::bad_alloc& e) {return 0;}
    return totalSize;
}

size_t ReadCallback(void* ptr, size_t size, size_t nmemb, void* stream) {
    std::istream* contentStream = static_cast<std::istream*>(stream);
    if (!contentStream->good()) return 0; // Stop if the stream is in a bad state
    contentStream->read(static_cast<char*>(ptr), size * nmemb);
    return contentStream->gcount(); // Return the number of bytes read
}

class CurlHandle {
public:
    CurlHandle() : handle(curl_easy_init()) {if (!handle) throw std::runtime_error("Failed to initialize CURL");}
    ~CurlHandle() {if (handle) curl_easy_cleanup(handle);}
    CURL* get() const {return handle;}
private:
    CURL* handle;
    CurlHandle(const CurlHandle&) = delete;
    CurlHandle& operator=(const CurlHandle&) = delete;
    CurlHandle(CurlHandle&&) = delete;
    CurlHandle& operator=(CurlHandle&&) = delete;
};

std::string fetchHttpContent(const std::string& url, long timeout = 0) {
    CurlHandle curlHandle; 
    CURL* curl = curlHandle.get();
    CURLcode res;
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    if(timeout) curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    res = curl_easy_perform(curl);
    bbassert (res == CURLE_OK, "Failed to fetch URL: " + url + ", error: " + curl_easy_strerror(res));
    return response;
}

std::string fetchFtpContent(const std::string& url, const std::string& username = "", const std::string& password = "", long timeout = 0) {
    CurlHandle curlHandle;  // Assume CurlHandle manages CURL initialization and cleanup.
    CURL* curl = curlHandle.get();
    CURLcode res;
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    if(timeout) curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    if (!username.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    }
    res = curl_easy_perform(curl);
    bbassert(res == CURLE_OK, "Failed to fetch FTP content: " + url + ", error: " + curl_easy_strerror(res));

    return response;
}


std::string fetchFtpsContent(const std::string& url, const std::string& username = "", const std::string& password = "", long timeout = 0) {
    CurlHandle curlHandle;
    CURL* curl = curlHandle.get();
    CURLcode res;
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);  // Enable SSL/TLS
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    if(timeout) curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    if (!username.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    res = curl_easy_perform(curl);
    bbassert(res == CURLE_OK, "Failed to fetch FTPS content: " + url + ", error: " + curl_easy_strerror(res));
    return response;
}

std::string fetchSftpContent(const std::string& url, const std::string& username = "", const std::string& password = "", long timeout = 0) {
    CurlHandle curlHandle; 
    CURL* curl = curlHandle.get();
    CURLcode res;
    std::string response;
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    if(timeout) curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    //curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    if (!username.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    }
    res = curl_easy_perform(curl);
    bbassert(res == CURLE_OK, "Failed to fetch SFTP content: " + url + ", error: " + curl_easy_strerror(res));
    return response;
}

void uploadSftpContent(const std::string& url, const std::string& content, const std::string& username = "", const std::string& password = "", long timeout = 0) {
    CurlHandle curlHandle; 
    CURL* curl = curlHandle.get();
    CURLcode res;
    std::istringstream contentStream(content);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &contentStream);
    if(timeout) curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    if (!username.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    }
    res = curl_easy_perform(curl);
    bbassert(res == CURLE_OK, "Failed to upload SFTP content to: " + url + ", error: " + curl_easy_strerror(res));
    curl_easy_cleanup(curl);
}


void uploadFtpsContent(const std::string& url, const std::string& content, const std::string& username = "", const std::string& password = "", long timeout = 0) {
    CurlHandle curlHandle;
    CURL* curl = curlHandle.get();
    CURLcode res;
    std::istringstream contentStream(content);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &contentStream);
    if(timeout) curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    if (!username.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    res = curl_easy_perform(curl);
    bbassert(res == CURLE_OK, "Failed to upload FTPS content to: " + url + ", error: " + curl_easy_strerror(res));
}

std::string fetchHttpsContent(const std::string& url, const std::string& username = "", const std::string& password = "", long timeout = 0) {
    CurlHandle curlHandle; 
    CURL* curl = curlHandle.get();
    CURLcode res;
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    if(timeout) curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    if (!username.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    res = curl_easy_perform(curl);
    bbassert(res == CURLE_OK, "Failed to fetch HTTPS content: " + url + ", error: " + curl_easy_strerror(res));
    return response;
}

std::string fetchHttpsContentWithToken(const std::string& url, const std::string& token) {
    CurlHandle curlHandle; 
    CURL* curl = curlHandle.get();
    CURLcode res;
    std::string response;
    struct curl_slist* headers = nullptr;
    std::string authHeader = "Authorization: Bearer " + token;
    headers = curl_slist_append(headers, authHeader.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    bbassert(res == CURLE_OK, "Failed to fetch HTTPS content: " + url + ", error: " + curl_easy_strerror(res));
    return response;
}

void uploadFtpContent(const std::string& url, const std::string& content, const std::string& username, const std::string& password, long timeout) {
    CurlHandle curlHandle; 
    CURL* curl = curlHandle.get();
    CURLcode res;
    std::istringstream contentStream(content);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &contentStream);
    if(timeout) curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    if(!username.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    }
    res = curl_easy_perform(curl);
    bbassert(res == CURLE_OK, "Failed to upload FTP content to: " + url + ", error: " + curl_easy_strerror(res));
}


BFile::BFile(const std::string& path_) : path(normalizeFilePath(path_)), size(0), Data(FILETYPE), contentsLoaded(false) {
    bbassert(isAllowedLocationNoNorm(path), "Access denied for path: " + path +
                                      "\n   \033[33m!!!\033[0m This is a safety measure imposed by Blombly."
                                      "\n       You need to add read permissions to a location containting the prefix with `!access \"location\"`."
                                      "\n       Permisions can only be granted this way from the virtual machine's entry point."
                                      "\n       They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.");
}

void BFile::loadContents() {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    bbassert(isAllowedLocationNoNorm(path), "Access denied for path: " + path +
                                      "\n   \033[33m!!!\033[0m This is a safety measure imposed by Blombly."
                                      "\n       You need to add read permissions to a location containting the prefix with `!access \"location\"`."
                                      "\n       Permisions can only be granted this way from the virtual machine's entry point."
                                      "\n       They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.");
    if (contentsLoaded) return;
    if (path.find("vfs://", 0) == 0) {
        std::lock_guard<std::recursive_mutex> lock(virtualFileSystemLock);
        if(virtualFileSystem.find(path)==virtualFileSystem.end()) bberror("Virtual file does not exist: " + path);
        std::istringstream file(virtualFileSystem[path]);
        std::string line;
        while (std::getline(file, line)) contents.push_back(line);
    } else if (path.find("http://", 0) == 0) {
        std::string httpContent = fetchHttpContent(path, timeout);
        bbassert(!httpContent.empty(), "Failed to fetch content from HTTP path: " + path);
        std::istringstream stream(httpContent);
        std::string line;
        while (std::getline(stream, line)) contents.push_back(line);
    } else if (path.find("https://", 0) == 0) {
        std::string httpContent = fetchHttpsContent(path, username, password, timeout);
        bbassert(!httpContent.empty(), "Failed to fetch content from HTTP path: " + path);
        std::istringstream stream(httpContent);
        std::string line;
        while (std::getline(stream, line)) contents.push_back(line);
    } else if (path.find("ftp://", 0) == 0) {
        std::string ftpContent = fetchFtpContent(path, username, password, timeout);
        bbassert(!ftpContent.empty(), "Failed to fetch content from FTP path: " + path);
        std::istringstream stream(ftpContent);
        std::string line;
        while (std::getline(stream, line)) contents.push_back(line);
    } else if (path.find("sftp://", 0) == 0) {
        std::string sftpContent = fetchSftpContent(path, username, password, timeout);
        bbassert(!sftpContent.empty(), "Failed to fetch content from SFTP path: " + path);
        std::istringstream stream(sftpContent);
        std::string line;
        while (std::getline(stream, line)) contents.push_back(line);
    } else if (path.find("ftps://", 0) == 0) {
        std::string ftpsContent = fetchFtpsContent(path, username, password, timeout);
        bbassert(!ftpsContent.empty(), "Failed to fetch content from FTPS path: " + path);
        std::istringstream stream(ftpsContent);
        std::string line;
        while (std::getline(stream, line)) contents.push_back(line);
    } else if (fs::is_directory(path)) {
        for (const auto& entry : fs::directory_iterator(path)) contents.push_back(entry.path().string());
    } else {
        std::ifstream file(path);
        bbassert(file.is_open(), "Failed to open file: " + path);
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
    bbassert(isAllowedLocationNoNorm(path), "Access denied for path: " + path +
                                      "\n   \033[33m!!!\033[0m This is a safety measure imposed by Blombly."
                                      "\n       You need to add read permissions to a location containting the prefix with `!access \"location\"`."
                                      "\n       Permisions can only be granted this way from the virtual machine's entry point."
                                      "\n       They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.");
    if (path.find("http://", 0) == 0) return true;
    if (path.find("https://", 0) == 0) return true;
    if (path.find("ftp://", 0) == 0) return true;
    if (path.find("sftp://", 0) == 0) return true;
    if (path.find("ftps://", 0) == 0) return true;
    if (path.find("vfs://", 0) == 0) virtualFileSystem.find(path) != virtualFileSystem.end();
    return fs::exists(path);
}

Result BFile::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    if (operation == AT && args->size == 2 && args->arg1->getType() == BB_INT) {
        loadContents();
        int64_t lineNum = static_cast<Integer*>(args->arg1.get())->getValue();
        if (lineNum < 0 || lineNum >= contents.size()) return std::move(Result(OUT_OF_RANGE));
        std::string lineContent = contents[lineNum];
        STRING_RESULT(lineContent);
    }
    if (operation == DIV && args->size == 2 && args->arg1->getType() == STRING) {
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
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        bbassert(isAllowedWriteLocationNoNorm(path), "Write access denied for path: " + path +
                                        "\n   \033[33m!!!\033[0m This is a safety measure imposed by Blombly."
                                        "\n       You need to add write permissions to a location containting the prefix with `!modify \"location\"`."
                                        "\n       Permisions can only be granted this way from the virtual machine's entry point."
                                        "\n       They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.");

        std::string newContents = args->arg1->toString(memory);
        
        if (path.find("vfs://", 0) == 0) {
            std::lock_guard<std::recursive_mutex> lock(virtualFileSystemLock);
            virtualFileSystem[path] = newContents;
        }
        else if (path.find("ftp://", 0) == 0) uploadFtpContent(path, newContents, username, password, timeout);
        else if (path.find("sftp://", 0) == 0) uploadSftpContent(path, newContents, username, password, timeout);
        else if (path.find("ftps://", 0) == 0) uploadFtpsContent(path, newContents, username, password, timeout);
        else {
            ensureWritePermissionsNoNorm(path);
            std::ofstream file(path, std::ios::trunc);
            bbassert(file.is_open(), "Failed to open file for writing: " + path);
            file << newContents;
            file.close();
        }

        contents.clear();
        std::istringstream stream(newContents);
        std::string line;
        while (std::getline(stream, line)) contents.push_back(line);
        size = contents.size();
        contentsLoaded = true;
        return std::move(Result(this));
    }
    if (args->size == 1) {
        if (operation == CLEAR) {
            std::lock_guard<std::recursive_mutex> lock(memoryLock);
            bbassert(isAllowedWriteLocationNoNorm(path), "Write access denied for path: " + path +
                                            "\n   \033[33m!!!\033[0m This is a safety measure imposed by Blombly."
                                            "\n       You need to add write permissions to a location containting the prefix with `!modify \"location\"`."
                                            "\n       Permisions can only be granted this way from the virtual machine's entry point."
                                            "\n       They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.");

            /*bbassert(path.find("http://", 0)!=0, "Cannot clear HTTP resource (it does not persist): " + path);
            bbassert(path.find("https://", 0)!=0, "Cannot clear HTTPS resource (it does not persist): " + path);
            bbassert(path.find("https://", 0)!=0, "Cannot clear FTP resource (it does not persist): " + path);*/
            if(path.find("http://", 0)==0 
                || path.find("https://", 0)==0 
                || path.find("ftp://", 0)==0 
                || path.find("sftp://", 0)==0
                || path.find("ftps://", 0)==0) {
                    bberror("Cannot clear a web resource: "+path);
                }
            bbassert(fs::exists(path), "Path does not exist: " + path);
            if (fs::is_regular_file(path)) {
                fs::remove(path);
                size = 0;
                contentsLoaded = false;
                contents.clear();
                return std::move(Result(this));
            } 
            else if (fs::is_directory(path)) {
                bbassert(fs::is_empty(path), "For safety, only empty directories can be cleared by the virtual machine, but this has contents: " + path);
                fs::remove(path);
                contents.clear();
                size = 0;
                contentsLoaded = false;
                return std::move(Result(this));
            } 
            else bberror("Path is not a regular file or directory: " + path);
        }
        if (operation == LEN) {
            loadContents();
            int64_t ret = contents.size();
            BB_INT_RESULT(ret);
        }
        if (operation == TOFILE) return std::move(Result(this));
        if (operation == TOSTR) STRING_RESULT(toString(memory));
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

    if (operation == PUT && args->size==3 && args->arg1->getType()==STRING) {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        std::string param = args->arg1->toString(memory);
        if(param=="username") {
            bbassert(args->arg2->getType()==STRING, param+" must a string");
            username = args->arg2->toString(memory);
        }
        else if(param=="password") {
            bbassert(args->arg2->getType()==STRING, param+" must a string");
            password = args->arg2->toString(memory);
        }
        else if(param=="timeout") {
            bbassert(args->arg2->getType()==BB_INT, param+" must an int");
            timeout = static_cast<Integer*>(args->arg2.get())->getValue();
        }
        else bberror("Only \"username\", \"password\", or \"timeout\" parameters can be set.");
        return std::move(Result(nullptr));
    }
    throw Unimplemented();
}
