#include "data/RestServer.h"
#include "data/Code.h"
#include "data/List.h"
#include "data/Struct.h"
#include "data/BString.h"
#include "data/Integer.h"
#include "BMemory.h"
#include "common.h"
#include <iostream>
#include <string>


extern std::recursive_mutex printMutex;

RestServer::RestServer(int port) : port_(port), context_(nullptr) {}

RestServer::~RestServer() {
    if (context_) {
        mg_stop(context_);
    }
}

std::string RestServer::toString() const {
    return "RestServer running on port " + std::to_string(port_);
}

int RestServer::getType() const {
    return SERVER;
}

std::shared_ptr<Data> RestServer::shallowCopy() const {
    return std::make_shared<RestServer>(port_);
}

void RestServer::runServer() {
    //std::cout << "Starting RestServer on port " << port_ << std::endl;

    const char* options[] = {
        "listening_ports", std::to_string(port_).c_str(),
        "num_threads", "4",  
        nullptr
    };

    context_ = mg_start(nullptr, 0, options);
    if (context_ == nullptr) {
        std::cerr << "Failed to start Civetweb server" << std::endl;
        return;
    }
    /*for (const auto& entry : routeHandlers_) {
        const std::string& route = entry.first;
        mg_set_request_handler(context_, route.c_str(), requestHandler, (void*)this);
    }*/
}

std::shared_ptr<Data> RestServer::implement(const OperationType operation, BuiltinArgs* args) {
    if (operation == PUT && args->size == 3 && args->arg1->getType() == STRING && (args->arg2->getType() == CODE || args->arg2->getType() == STRUCT)) {
        std::lock_guard<std::recursive_mutex> lock(serverModification);
        std::string route = std::static_pointer_cast<BString>(args->arg1)->toString();
        if(routeHandlers_.find(route)==routeHandlers_.end())
            mg_set_request_handler(context_, route.c_str(), requestHandler, (void*)this);  // the handler will always be the same, so it doesn't matter if a new runnable takes its place
        routeHandlers_[route] = args->arg2;
        return nullptr;
    }
    throw Unimplemented();
}

std::shared_ptr<Data> RestServer::executeCode(std::shared_ptr<Data> called, BuiltinArgs* args) {
    if(called->getType()==STRUCT) {
        auto strct = std::static_pointer_cast<Struct>(called);
        auto val = strct->getMemory()->getOrNullShallow(variableManager.callId);
        bbassert(val && val->getType()==CODE, "Struct was called like a method but has no implemented code for `\\call`.");
        called = std::move(val);
    }
    bbassert(called->getType()==CODE, "Internally corrupted server callable is neither code nor struct (this message should never appear due to earlier error checking)");
    auto code = std::static_pointer_cast<Code>(called);
    auto mem = std::make_shared<BMemory>(code->getDeclarationMemory(), LOCAL_EXPECTATION_FROM_CODE(code));
    auto listArgs = std::make_shared<BList>();
    mem->unsafeSet(variableManager.argsId, listArgs, nullptr);

    bool hasReturned = false;
    BuiltinArgs callArgs;
    auto result = executeBlock(code, mem, hasReturned, callArgs);

    if (!hasReturned) 
        bberror("Route handler did not reach a return statement.");
    if (!hasReturned) 
        bberror("Route handler returned a missing value.");

    return result;   
}

int RestServer::requestHandler(struct mg_connection* conn, void* cbdata) {
    RestServer* server = static_cast<RestServer*>(cbdata);
    const mg_request_info* req_info = mg_get_request_info(conn);
    std::string route(req_info->request_uri);

    try {
        auto it = server->routeHandlers_.find(route);
        if (it != server->routeHandlers_.end()) {
            BuiltinArgs args;
            args.size = 0;
            std::shared_ptr<Data> result = server->executeCode(it->second, &args);
            std::string response = result->toString();
            mg_printf(conn,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %lu\r\n"
                    "\r\n%s",
                    response.length(), response.c_str());

            return 200;
        }
    }
    catch (const BBError& e) {
        std::lock_guard<std::recursive_mutex> lock(printMutex);
        std::string response = e.what();
        std::cerr << e.what() << "\n";
        mg_printf(conn, "HTTP/1.1 500 Internal error\r\nContent-Length: 0\r\n\r\n");
        return 500;
    }

    mg_printf(conn, "HTTP/1.1 404 Route not found\r\nContent-Length: 0\r\n\r\n");
    return 404;
}
