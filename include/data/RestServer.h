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
    RestServer(int port);
    ~RestServer();

    // Overrides from Data
    std::string toString() const override;
    int getType() const override;
    std::shared_ptr<Data> shallowCopy() const override;

    void runServer();
    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;

private:
    int port_;
    struct mg_context* context_;
    std::unordered_map<std::string, std::shared_ptr<Data>> routeHandlers_;
    std::shared_ptr<Data> executeCodeWithMemory(std::shared_ptr<Data> called, const std::shared_ptr<BMemory>& memory) const;
    static int requestHandler(struct mg_connection* conn, void* cbdata);
    std::recursive_mutex serverModification;
};

#endif // RESTSERVER_H
