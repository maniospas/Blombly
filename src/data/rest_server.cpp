#include "data/RestServer.h"
#include "data/Code.h"
#include "data/List.h"
#include "data/Struct.h"
#include "data/BString.h"
#include "data/Integer.h"
#include "data/Boolean.h"
#include "BMemory.h"
#include "common.h"
#include <iostream>
#include <string>
#include <sstream>


std::vector<std::string> splitRoute(const std::string& route) {
    std::vector<std::string> routeParts;
    std::stringstream ss(route);
    std::string segment;
    while (std::getline(ss, segment, '/')) {
        if (!segment.empty()) {
            routeParts.push_back(segment);
        }
    }
    return routeParts;
}


extern std::recursive_mutex printMutex;

RestServer::RestServer(int port) : port_(port), context_(nullptr), Data(SERVER) {}

RestServer::~RestServer() {
    if (context_) {
        mg_stop(context_);
    }
}

std::string RestServer::toString() const {
    return "Server on port " + std::to_string(port_);
}

void RestServer::runServer() {
    const char* options[] = {
        "listening_ports", std::to_string(port_).c_str(),
        "num_threads", "4",  
        nullptr
    };
    context_ = mg_start(nullptr, 0, options);
    if (context_ == nullptr) 
        bberror("Failed to start server on port " + std::to_string(port_));
    mg_set_request_handler(context_, "/", requestHandler, (void*)this);
}

Data* RestServer::implement(const OperationType operation, BuiltinArgs* args) {
    if (operation == PUT && args->size == 3 && args->arg1->getType() == STRING &&
        (args->arg2->getType() == CODE || args->arg2->getType() == STRUCT)) {
        std::lock_guard<std::recursive_mutex> lock(serverModification);
        std::string route = static_cast<BString*>(args->arg1)->toString();
        routeHandlers_[route] = args->arg2;
        return nullptr;
    }
    throw Unimplemented();
}


Data* RestServer::executeCodeWithMemory(Data* called, BMemory* memory) const {
    if(called->getType()==STRUCT) {
        auto strct = static_cast<Struct*>(called);
        auto val = strct->getMemory()->getOrNullShallow(variableManager.callId);
        bbassert(val && val->getType()==CODE, "Struct was called like a method but has no implemented code for `\\call`.");
        called = (val);
    }
    bbassert(called->getType()==CODE, "Internally corrupted server callable is neither code nor struct (this message should never appear due to earlier error checking)");
    auto code = static_cast<Code*>(called);
    memory->detach(code->getDeclarationMemory());
    auto listArgs = new BList();
    memory->unsafeSet(variableManager.argsId, listArgs, nullptr);

    bool hasReturned = false;
    auto result = executeBlock(code, memory, hasReturned);

    if (!hasReturned) 
        bberror("Server route handler did not reach a return statement.");
    if (!result) 
        bberror("Server route handler returned no value.");
    if (result->getType()!=Datatype::STRING) 
        bberror("Server route handler did not return a string.");

    return result;   
}

int RestServer::requestHandler(struct mg_connection* conn, void* cbdata) {
    RestServer* server = static_cast<RestServer*>(cbdata);
    const mg_request_info* req_info = mg_get_request_info(conn);


    std::string route(req_info->request_uri);
    std::vector<std::string> routeParts = splitRoute(route);
    for (const auto& entry : server->routeHandlers_) {
        std::vector<std::string> registeredRouteParts = splitRoute(entry.first);
        if (routeParts.size() == registeredRouteParts.size()) {
            auto mem = new BMemory(nullptr, (int)routeParts.size()/2+1);
            bool isMatch = true;
            for (size_t i = 0; i < routeParts.size(); ++i) {
                if (registeredRouteParts[i][0] == '<' && registeredRouteParts[i][registeredRouteParts[i].size()-1] == '>') {
                    std::string varName = registeredRouteParts[i].substr(1, registeredRouteParts[i].size()-2);
                    std::string varValue = routeParts[i];
                    // TODO: prevent invalid variable names
                    int varId = variableManager.getId(varName);
                    mem->unsafeSet(varId, new BString(varValue), nullptr);
                    //mem->setFinal(varId);
                } 
                else if (registeredRouteParts[i] != routeParts[i]) {
                    isMatch = false;
                    break;
                }
            }
            if (!isMatch) 
                continue;

            try {
                if(req_info->request_uri) {
                    mem->unsafeSet(variableManager.getId("uri"), new BString(req_info->request_uri), nullptr);
                    //mem->setFinal(variableManager.getId("uri"));
                }
                if(req_info->query_string) {
                    mem->unsafeSet(variableManager.getId("query"), new BString(req_info->query_string), nullptr);
                    //mem->setFinal(variableManager.getId("query"));
                }
                if(req_info->request_method) {
                    mem->unsafeSet(variableManager.getId("method"), new BString(req_info->request_method), nullptr);
                    //mem->setFinal(variableManager.getId("method"));
                }
                if(req_info->http_version) {
                    mem->unsafeSet(variableManager.getId("http"), new BString(req_info->http_version), nullptr);
                    //mem->setFinal(variableManager.getId("http"));
                }
                mem->unsafeSet(variableManager.getId("ip"), new BString(req_info->remote_addr), nullptr);
                mem->unsafeSet(variableManager.getId("ssl"), new Boolean(req_info->is_ssl), nullptr);
                //mem->setFinal(variableManager.getId("ip"));
                //mem->setFinal(variableManager.getId("ssl"));

                if(req_info->content_length>0) {
                    int contentLength = req_info->content_length;
                    std::vector<char> bodyData(contentLength + 1);
                    int bytesRead = mg_read(conn, &bodyData[0], contentLength);
                    if (bytesRead > 0) {
                        bodyData[bytesRead] = '\0';
                        std::string requestBody(&bodyData[0]);
                        mem->unsafeSet(variableManager.getId("content"), new BString(requestBody), nullptr);
                    }
                }

                Data* result = server->executeCodeWithMemory(entry.second, mem);
                std::string response = result->toString();

                std::string html_prefix = "<!DOCTYPE html>";
                if(html_prefix.size()>=html_prefix.length() && response.substr(0, html_prefix.length())==html_prefix) {
                    mg_printf(conn,
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: %lu\r\n"
                            "\r\n%s",
                            response.length(), response.c_str());
                }
                else
                    mg_printf(conn,
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: %lu\r\n"
                            "\r\n%s",
                            response.length(), response.c_str());
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