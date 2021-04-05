#include <sys/socket.h>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <arpa/inet.h>
#include "./util.hpp"
#include "./socket.hpp"

Socket::~Socket() {}

Socket::Socket(int type) {
    int tempfd = socket(AF_INET, type, 0);
    if (tempfd == -1) {
        throw SocketError("Socket" + str_err());
    }

    sockfd = tempfd;
}

void Socket::setOpt(int option) {
    int en = 1;
    if(setsockopt(sockfd, SOL_SOCKET, option, (char*)&en, sizeof(en)) == -1) {
        throw SocketError("setOpt: " + str_err());
    }
}

void Socket::bindSock(in_addr_t ip, int port) {
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(ip);
    sin.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&sin, sizeof(sin)) == -1) {
        throw SocketError("bindSock: " + str_err());
    }
}

int Socket::fd() { return sockfd; }

Address Socket::addr() {
    return {inet_ntoa(sin.sin_addr), ntohs(sin.sin_port)};
}