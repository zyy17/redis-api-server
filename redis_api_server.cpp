#include <iostream>
#include <vector>
#include <memory>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <deps/resp/resp/encoder.hpp>
#include <deps/resp/resp/buffer.hpp>
#include <deps/resp/resp/decoder.hpp>

#include "ae.h"
#include "anet.h"
#include "redis_api_server.h"
#include "LogCabin/Client.h"

using LogCabin::Client::Cluster;
using LogCabin::Client::Tree;

static std::shared_ptr<Tree> pTree = nullptr;

void writeToClient(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    char *buffer = static_cast<char*>(clientdata);
    write(fd, buffer, strlen(buffer));
    free(buffer);
    aeDeleteFileEvent(loop, fd, mask);
}

void reply(int fd, char *send_buffer, std::string content) {
    std::cout << "reply:" << content << std::endl;
    memcpy(send_buffer, content.c_str(), content.length());
    write(fd, send_buffer, content.length());
}

std::string generate_bulk_string(const std::vector<std::string> &s) {
    std::string bulk_str = "*" + std::to_string(s.size());
    bulk_str = bulk_str + "\r\n";
    for (auto it = s.begin(); it != s.end(); ++it) {
        bulk_str += "$" + std::to_string(it->length()) + "\r\n" + *it + "\r\n";
    }
    return bulk_str;
}

std::vector<std::string> split_list_elements(std::string &original) {
    std::string s(original);
    std::vector<std::string> elements;

    std::string delimiter1 = ",";
    std::string delimiter2 = ":";
    std::string token;

    size_t pos = 0;
    int counter = 0;

    while ((pos = s.find(delimiter1)) != std::string::npos) {
        token = s.substr(0, pos);
        elements.push_back(token);
        s.erase(0, pos + delimiter1.length());
    }

    for (auto it = elements.begin(); it != elements.end(); it++) {
        counter = 0;
        while ((pos = it->find(delimiter2)) != std::string::npos && counter <= 4) {
            counter++;
            token = it->substr(0, pos);
            it->erase(0, pos + delimiter2.length());
        }
    }

    return elements;
}

// FIXME: terrible memory management
void readFromClient(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    const int buffer_size = 1024;
    char recv_buffer[buffer_size];
    char send_buffer[buffer_size];

    // clear buffer in entry
    bzero(recv_buffer, buffer_size);
    bzero(send_buffer, buffer_size);

    ssize_t size = read(fd, recv_buffer, buffer_size);
    if (size <= 0) {
        aeDeleteFileEvent(loop, fd, mask);
    } else {
        if (size > 0) {
            resp::decoder dec;
            resp::encoder<resp::buffer> enc;
            std::vector<resp::buffer> buffers;
            resp::result res = dec.decode(recv_buffer, std::strlen(recv_buffer));
            resp::unique_value rep = res.value();

            if (rep.type() == resp::ty_array) {
                // FIXME: it will crash when array size is 8
                resp::unique_array<resp::unique_value> arr = rep.array();
                if (arr[0].bulkstr() == "SET" || arr[0].bulkstr() == "set") {
                    std::cout << "SET key: " << arr[1].bulkstr().data()
                              << " , value: " << arr[2].bulkstr().data() << std::endl;
                    // you can add your handler here
                    reply(fd, send_buffer, "+OK\r\n");
                } else if (arr[0].bulkstr() == "GET" || arr[0].bulkstr() == "get") {
                    std::cout << "GET key: " << arr[1].bulkstr().data() << std::endl;
                    // you can add your handler here
                    buffers = enc.encode("foo");
                    reply(fd, send_buffer, buffers.data()[0].data());
                } else if (arr[0].bulkstr() == "rpush" || arr[0].bulkstr() == "RPUSH") {
                    std::string rpush_key = arr[1].bulkstr().data();
                    std::cout << "RPUSH key: " << rpush_key << std::endl;
                    try {
                        for (int i = 2; i < arr.size(); i++) {
                            std::cout << arr[i].bulkstr().data() << std::endl;
                            pTree->rpushEx(rpush_key, arr[i].bulkstr().data());
                        }
                    } catch (const LogCabin::Client::Exception& e) {
                        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                                  << e.what()
                                  << std::endl;
                        exit(1);
                    }
                    reply(fd, send_buffer, "+OK\r\n");
                } else if (arr[0].bulkstr() == "lrange" || arr[0].bulkstr() == "LRANGE") {
                    std::string lrange_key = arr[1].bulkstr().data();
                    std::cout << "LRANGE key: " << lrange_key << std::endl;
                    std::string element_list;
                    try {
                        element_list = pTree->readEx(lrange_key);
                    } catch (const LogCabin::Client::Exception &e) {
                        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                                  << e.what()
                                  << std::endl;
                        exit(1);
                    }
                    reply(fd, send_buffer,
                          std::move(generate_bulk_string(std::move(split_list_elements(element_list)))));
                } else if (arr[0].bulkstr() == "ltrim" || arr[0].bulkstr() == "LTRIM") {
                    std::string ltrim_key = arr[1].bulkstr().data();
                    std::string length = arr[2].bulkstr().data();
                    std::cout << "LTRIM key: " << ltrim_key << " , length: " << length << std::endl;
                    try {
                        pTree->ltrim(ltrim_key, length);
                    } catch (const LogCabin::Client::Exception &e) {
                        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                                  << e.what()
                                  << std::endl;
                        exit(1);
                    }
                    reply(fd, send_buffer, "+OK\r\n");
                } else {
                    std::string pre_part("-ERR unknown command '");
                    std::string unknown_command(arr[0].bulkstr().data());
                    reply(fd, send_buffer, pre_part + unknown_command + "'\r\n");
                }
            } else {
                reply(fd, send_buffer, "+OK\r\n");
            }
        } else {
            // do nothing
        }
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
    Cluster cluster("127.0.0.1:5254");
    Tree tree = cluster.getTree();
    pTree = std::make_shared<Tree>(tree);
    RedisAPIServer server = {6380, 1024, "0.0.0.0", writeToClient, readFromClient, acceptTcpHandler};
    server.run();
}
