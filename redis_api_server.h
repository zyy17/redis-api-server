#ifndef REDIS_API_SERVER_REDIS_API_SERVER_H
#define REDIS_API_SERVER_REDIS_API_SERVER_H

#include "ae.h"
#include "anet.h"

#include <iostream>
#include <cassert>
#include <string>

class RedisAPIServer {
public:
    RedisAPIServer() = default;
    RedisAPIServer(const int &service_port, const int &set_size, const std::string &bind_addr,
                   aeFileProc *writable_handler, aeFileProc *readable_handler, aeFileProc *accept_handler) :
                  _service_port(service_port), _set_size(set_size), _bind_addr(bind_addr),
                  loop(aeCreateEventLoop(set_size)),
                  _writable_handler(writable_handler), _readable_handler(readable_handler), _accept_handler(accept_handler)
    { };
    bool run();
    ~RedisAPIServer()
    {
        // handling the resources
        aeDeleteEventLoop(loop);
    }

private:
    int _service_port = 8080;
    int _set_size = 1024;
    std::string _bind_addr = "0.0.0.0";
    aeEventLoop *loop = nullptr;
    aeFileProc *_writable_handler = nullptr;
    aeFileProc *_readable_handler = nullptr;
    aeFileProc *_accept_handler = nullptr;
};

bool RedisAPIServer::run()
{
    int ipfd = anetTcpServer(nullptr, _service_port, const_cast<char *>(_bind_addr.c_str()), 0);
    assert(ipfd != ANET_ERR);

    int ret = aeCreateFileEvent(loop, ipfd, AE_READABLE, _accept_handler, nullptr);
    assert(ret != AE_ERR);

    aeMain(loop);
}

#endif //REDIS_API_SERVER_REDIS_API_SERVER_H