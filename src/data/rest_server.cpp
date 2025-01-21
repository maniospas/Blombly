#include "data/RestServer.h"
#include "data/Code.h"
#include "data/List.h"
#include "data/Struct.h"
#include "data/BString.h"
#include "data/Jitable.h"
#include "BMemory.h"
#include "common.h"
#include <iostream>
#include <string>
#include <sstream>
#include "interpreter/functional.h"


std::vector<std::string> splitRoute(const std::string& route) {
    std::vector<std::string> routeParts;
    std::stringstream ss(route);
    std::string segment;
    while (std::getline(ss, segment, '/')) if (!segment.empty()) routeParts.push_back(segment);
    return routeParts;
}

extern std::recursive_mutex printMutex;
int RestServer::resultType = variableManager.getId("type");

RestServer::RestServer(BMemory* attachedMemory, int port) : port_(port), context_(nullptr), Data(SERVER), attachedMemory(attachedMemory) {runServer();}
RestServer::RestServer(BMemory* attachedMemory, RestServer* prototype) : port_(prototype->port_), context_(prototype->context_), Data(SERVER), attachedMemory(attachedMemory) {
    routeHandlers_ = std::move(prototype->routeHandlers_);
    mg_set_request_handler(context_, "/", requestHandler, (void*)this);
}
std::string RestServer::toString(BMemory* memory){return "Server on port " + std::to_string(port_);}
RestServer::~RestServer() { 
    if (context_) mg_stop(context_); 
}

Result RestServer::put(BMemory* callerMemory,const DataPtr& route, const DataPtr& code) {
    bbassert(context_, "Server has stopped; it has been moved or cleared");
    bbassert(route.existsAndTypeEquals(STRING), "Server route should be a string");
    bbassert(code.existsAndTypeEquals(CODE) || code.existsAndTypeEquals(STRUCT), "Server route should assign a code block or struct");
    std::lock_guard<std::recursive_mutex> lock(serverModification);
    std::string actualRoute = static_cast<BString*>(route.get())->toString(nullptr);
    Data* actualCode = static_cast<Data*>(code.get());
    if(routeHandlers_[actualRoute]!=actualCode) {
        if(routeHandlers_[actualRoute]) routeHandlers_[actualRoute]->removeFromOwner();
        routeHandlers_[actualRoute] = actualCode;
        actualCode->addOwner();
    }
    return std::move(Result(nullptr));
}

void RestServer::stop() {
    std::lock_guard<std::recursive_mutex> lock(serverModification);
    if (context_) mg_stop(context_); 
    context_ = nullptr;
    routeHandlers_.clear();
}


void RestServer::runServer() {
    const char* options[] = {
        "listening_ports", std::to_string(port_).c_str(),
        "num_threads", "4",  
        nullptr
    };
    context_ = mg_start(nullptr, 0, options);
    if (context_ == nullptr) bberror("Failed to start server on port " + std::to_string(port_));
    mg_set_request_handler(context_, "/", requestHandler, (void*)this);
}


void RestServer::clear(BMemory* callerMemory) {stop();}
Result RestServer::move(BMemory* callerMemory) {
    std::lock_guard<std::recursive_mutex> lock(serverModification);
    RestServer* ret = new RestServer(callerMemory, this);
    routeHandlers_ = std::unordered_map<std::string, Data*>();
    context_ = nullptr;
    return std::move(Result(ret));
}


Result RestServer::executeCodeWithMemory(DataPtr called, BMemory* memory) const {
    if(called->getType()==STRUCT) {
        auto strct = static_cast<Struct*>(called.get());
        auto val = strct->getMemory()->getOrNullShallow(variableManager.callId);
        bbassert(val.existsAndTypeEquals(CODE), "Struct was called like a method but has no implemented code for `call`.");
        //static_cast<Code*>(val)->scheduleForParallelExecution = false; // struct calls are never executed in parallel
        memory->codeOwners[static_cast<Code*>(val.get())] = static_cast<Struct*>(called.get());
        called = (val);
    }
    bbassert(called->getType()==CODE, "Internally corrupted server callable is neither code nor struct (this message should never appear due to earlier error checking)");
    Code* code = static_cast<Code*>(called.get());


    BMemory newMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
    DataPtr result;
    //newMemory.detach(code->getDeclarationMemory());
    //newMemory.detach(memory);
    auto it = memory->codeOwners.find(code);
    DataPtr thisObj = (it != memory->codeOwners.end() ? it->second->getMemory() : memory)->getOrNull(variableManager.thisId, true);
    if(thisObj.exists()) newMemory.set(variableManager.thisId, thisObj);
    std::unique_lock<std::recursive_mutex> executorLock;
    if(thisObj.exists()) {
        bbassert(thisObj->getType()==STRUCT, "Internal error: `this` was neither a struct nor missing (in the last case it would have been replaced by the scope)");
        //if(!forceStayInThread) 
        executorLock = std::unique_lock<std::recursive_mutex>(static_cast<Struct*>(thisObj.get())->memoryLock);
    }
    newMemory.allowMutables = false;
    bool forceStayInThread = thisObj.exists(); // overwrite the option
    
    ExecutionInstance executor(code, &newMemory, forceStayInThread);
    Result returnedValue = executor.run(code);
    newMemory.detach(nullptr);
    result = returnedValue.get();
    if(thisObj.exists()) newMemory.setToNullIgnoringFinals(variableManager.thisId);

    bbassert(executor.hasReturned(), "Server route handler did not reach a return statement.");
    bbassert(result.exists(), "Server route handler returned no value.");
    return std::move(returnedValue);   

}

int RestServer::requestHandler(struct mg_connection* conn, void* cbdata) {
    RestServer* server = static_cast<RestServer*>(cbdata);
    const mg_request_info* req_info = mg_get_request_info(conn);

    std::string route(req_info->request_uri);
    std::vector<std::string> routeParts = splitRoute(route);
    for (const auto& entry : server->routeHandlers_) {
        std::vector<std::string> registeredRouteParts = splitRoute(entry.first);
        if (routeParts.size() == registeredRouteParts.size()) {
            BMemory mem(server->attachedMemory, (int)routeParts.size()/2+1);
            mem.allowMutables = false;
            bool isMatch = true;
            for (size_t i = 0; i < routeParts.size(); ++i) {
                if (registeredRouteParts[i][0] == '<' && registeredRouteParts[i][registeredRouteParts[i].size()-1] == '>') {
                    std::string varName = registeredRouteParts[i].substr(1, registeredRouteParts[i].size()-2);
                    std::string varValue = routeParts[i];
                    // TODO: prevent invalid variable names
                    int varId = variableManager.getId(varName);
                    mem.set(varId, new BString(varValue));
                    //mem.setFinal(varId);
                } 
                else if (registeredRouteParts[i] != routeParts[i]) {
                    isMatch = false;
                    break;
                }
            }
            if (!isMatch) continue;

            try {
                if(req_info->request_uri) mem.set(variableManager.getId("server::uri"), new BString(req_info->request_uri));
                if(req_info->query_string) mem.set(variableManager.getId("server::query"), new BString(req_info->query_string));
                if(req_info->request_method) mem.set(variableManager.getId("server::method"), new BString(req_info->request_method));
                if(req_info->http_version) mem.set(variableManager.getId("server::http"), new BString(req_info->http_version));
                mem.set(variableManager.getId("server::ip"), new BString(req_info->remote_addr));
                mem.set(variableManager.getId("server::ssl"), (bool)req_info->is_ssl);
                //mem.setFinal(variableManager.getId("ip"));
                //mem.setFinal(variableManager.getId("ssl"));

                if(req_info->content_length>0) {
                    int contentLength = req_info->content_length;
                    std::vector<char> bodyData(contentLength + 1);
                    int bytesRead = mg_read(conn, &bodyData[0], contentLength);
                    if (bytesRead > 0) {
                        bodyData[bytesRead] = '\0';
                        std::string requestBody(&bodyData[0]);
                        mem.set(variableManager.getId("server::content"), new BString(requestBody));
                    }
                }

                Result result_ = server->executeCodeWithMemory(entry.second, &mem);
                DataPtr result = result_.get();
                if(result->getType()==STRUCT) {
                    Struct* resultStruct = static_cast<Struct*>(result.get());
                    //DataPtr resultContentData = resultStruct->getMemory()->get(resultContent);
                    //bbassert(resultContentData, "Route returned a struct without a `content` field.");
                    std::string response = result->toString(&mem);
                    DataPtr resultTypeData = resultStruct->getMemory()->getOrNull(resultType, true);
                    bbassert(resultTypeData.exists(), "Server route returned a struct without a `type` field (it return nothing).");
                    bbassert(resultTypeData->getType()==Datatype::STRING, "Server route `type` field was not a string (type is not a string).");

                    mg_printf(conn,
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: %s\r\n"
                            "Content-Length: %lu\r\n"
                            "\r\n%s",
                            resultTypeData->toString(&mem).c_str(),
                            response.length(), response.c_str());
                }
                else {
                    bbassert(result->getType()==Datatype::STRING, "Server route handler did not return a string or struct with `str` and `type` (it returned neither a struct nor a string).");
                    std::string response = result->toString(&mem);
                    /*std::string html_prefix = "<!DOCTYPE html>";
                    if(html_prefix.size()>=html_prefix.length() && response.substr(0, html_prefix.length())==html_prefix) {
                        mg_printf(conn,
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/html\r\n"
                                "Content-Length: %lu\r\n"
                                "\r\n%s",
                                response.length(), response.c_str());
                    }
                    else*/
                    mg_printf(conn,
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: %lu\r\n"
                            "\r\n%s",
                            response.length(), response.c_str());
                }
                return 200;
            } 
            catch (const BBError& e) {
                std::lock_guard<std::recursive_mutex> lock(printMutex);
                std::cerr << e.what() << "\n";
                mg_printf(conn, "HTTP/1.1 500 Internal error\r\nContent-Length: 0\r\n\r\n");
                return 500;
            }
        }
    }

    mg_printf(conn, "HTTP/1.1 404 Route not found\r\nContent-Length: 0\r\n\r\n");
    return 404;
}