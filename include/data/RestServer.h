#ifndef RESTSERVER_H
#define RESTSERVER_H

#include "data/Data.h"
#include "civetweb.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>


class RestServer : public Data {
public:
    RestServer(BMemory* attachedMemory, int port);
    ~RestServer();

    std::string toString(BMemory* memory)override;
    void clear(BMemory* callerMemory) override;
    Result move(BMemory* callerMemory) override;
    Result put(BMemory* callerMemory,const DataPtr& route, const DataPtr& code) override;
private:
    RestServer(BMemory* attachedMemory, RestServer* prototype);
    void stop();
    void runServer();
    int port_;
    struct mg_context* context_;
    BMemory* attachedMemory;
    static int resultType;
    std::unordered_map<std::string, Data*> routeHandlers_;
    Result executeCodeWithMemory(DataPtr called, BMemory* memory) const;
    static int requestHandler(struct mg_connection* conn, void* cbdata);
    std::recursive_mutex serverModification;
    virtual void removeFromOwner() {
        if((--referenceCounter)==0) delete this; 
        else {
            stop();
            bberror(toString(nullptr)+" stopped because it was removed from at least one memory context.\n"
                    "Not stopping would be unsafe because servers run on the memory context in which they are created.\n"
                    "Use `move` to properly transfer ownership of the routes and connections to one new server\n"
                    "(for example if passing it as a function argument)  or `clear` to properly stop the server.");
        }
    }

};

#endif // RESTSERVER_H
