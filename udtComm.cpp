#include "udtComm.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <netdb.h>
#include <udt/udt.h>

double getSystemTime() {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec + 1e-9*time.tv_nsec;
}

UDTSOCKET udtConnect(const char* host, const char* port) {
    addrinfo hints;
    addrinfo *res;

    memset(&hints, 0, sizeof(hints));

    // hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = 0;
    if ((err = getaddrinfo(host, port, &hints, &res)) != 0) {
        std::cerr << "failed to set up addrinfo: " << gai_strerror(err) << std::endl;
        return UDT::ERROR;
    }

    UDTSOCKET client = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    UDT::setsockopt(client, 0, UDT_MSS, new int(9000), sizeof(int));

    if (UDT::ERROR == UDT::connect(client, res->ai_addr, res->ai_addrlen)) {
        std::cerr << "connect: " << UDT::getlasterror().getErrorMessage() << std::endl;
        return UDT::ERROR;
    }

    return client;
}

UDTSOCKET udtOpenServer(const char* port) {
    addrinfo hints;
    addrinfo *res;

    memset(&hints, 0, sizeof(hints));

    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        std::cerr << "failed to set up addrinfo" << std::endl;
        return UDT::ERROR;
    }

    UDTSOCKET server = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    UDT::setsockopt(server, 0, UDT_MSS, new int(9000), sizeof(int));
    UDT::setsockopt(server, 0, UDT_LINGER, new int(0), sizeof(int));

    if (UDT::ERROR == UDT::bind(server, res->ai_addr, res->ai_addrlen)) {
        std::cerr << "bind: " << UDT::getlasterror().getErrorMessage() << std::endl;
        return UDT::ERROR;
    }

    return server;
}

int udtRecv(UDTSOCKET sock, char *buf, int size) {
    if (buf == NULL) {
        return -1;
    }

    int rsize = 0;
    int rs;
    while (rsize < size) {
        if (UDT::ERROR == (rs = UDT::recv(sock, buf + rsize, size - rsize, 0))) {
            std::cerr << "recv: " << UDT::getlasterror().getErrorMessage() << std::endl;
            return -1;
        }

        rsize += rs;
    }

    return rsize;
}

int udtSend(UDTSOCKET sock, char *buf, int size) {
    int ssize = 0;
    int ss;

    while (ssize < size) {
        if (UDT::ERROR == (ss = UDT::send(sock, buf + ssize, size - ssize, 0))) {
            std::cerr << "send: " << UDT::getlasterror().getErrorMessage() << std::endl;
            return -1;
        }

        ssize += ss;
    }

    return ssize;
}