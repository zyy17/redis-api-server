#include <iostream>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <deps/resp/resp/encoder.hpp>
#include <deps/resp/resp/buffer.hpp>
#include <deps/resp/resp/decoder.hpp>

#include "ae.h"
#include "anet.h"
#include "redis_api_server.h"

void writeToClient(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    char *buffer = static_cast<char*>(clientdata);
    write(fd, buffer, strlen(buffer));
    free(buffer);
    aeDeleteFileEvent(loop, fd, mask);
}

void reply(int fd, char *send_buffer, std::string content) {
    memcpy(send_buffer, content.c_str(), content.length());
    write(fd, send_buffer, content.length());
}

// FIXME: terrible memory management
void readFromClient(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    const int buffer_size = 1024;
    char recv_buffer[buffer_size];
    char send_buffer[buffer_size];
    bzero(recv_buffer, buffer_size);
    bzero(send_buffer, buffer_size);

    ssize_t size = read(fd, recv_buffer, buffer_size);

    std::cout << "recv: " << recv_buffer << std::endl;
    resp::decoder dec;
    resp::encoder<resp::buffer> enc;
    std::vector<resp::buffer> buffers;

    resp::result res = dec.decode(recv_buffer, std::strlen(recv_buffer));
    resp::unique_value rep = res.value();
    if (rep.type() == resp::ty_array) {
        resp::unique_array<resp::unique_value> arr = rep.array();
        if (arr[0].bulkstr() == "COMMAND") {
            reply(fd, send_buffer, "-ERR unknown command 'COMMAND'\r\n");
        } else if (arr[0].bulkstr() == "SET" || arr[0].bulkstr() == "set") {
            std::cout << "Set key: " << arr[1].bulkstr().data()
                      << " , value: " << arr[2].bulkstr().data() << std::endl;
            reply(fd, send_buffer, "+OK\r\n");
        } else if (arr[0].bulkstr() == "GET" || arr[0].bulkstr() == "get") {
            std::cout << "Get key: " << arr[1].bulkstr().data() << std::endl;
            buffers = enc.encode("foo");
            reply(fd, send_buffer, buffers.data()[0].data());
        }
    } else {
        reply(fd, send_buffer, "+OK\r\n");
    }
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
