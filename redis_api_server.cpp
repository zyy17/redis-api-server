#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "redis_api_server.h"

void writeToClient(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    char *buffer = static_cast<char*>(clientdata);
    write(fd, buffer, strlen(buffer));
    free(buffer);
    aeDeleteFileEvent(loop, fd, mask);
}

void readFromClient(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    int buffer_size = 1024;
    char *buffer = static_cast<char*>(malloc(sizeof(char) * buffer_size));
    bzero(buffer, buffer_size);
    int size = read(fd, buffer, buffer_size);

    // FIXME: when disconnect the tcp, it will print lots of log
    std::cout << "content: " << buffer << std::endl;
}

void acceptTcpHandler(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    int client_port, client_fd;
    char client_ip[128];

    // create client socket
    client_fd = anetTcpAccept(nullptr, fd, client_ip, 128, &client_port);
    std::cout << "Accepted " << client_ip << ":" << client_port << std::endl;

    // set client socket non-block
    anetNonBlock(nullptr, client_fd);

    // regist on message callback
    int ret;
    ret = aeCreateFileEvent(loop, client_fd, AE_READABLE, readFromClient, nullptr);
    assert(ret != AE_ERR);
}

int main()
{
    RedisAPIServer server = {6380, 1024, "0.0.0.0", writeToClient, readFromClient, acceptTcpHandler};
    server.run();
    return 0;
}
