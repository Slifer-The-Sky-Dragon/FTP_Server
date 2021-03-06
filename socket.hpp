#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

#include <string>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdexcept>
#include <utility>

typedef struct sockaddr_in SockaddrIn;
typedef std::pair<std::string, int> Address;

const std::string SOCKET = "Socket::";

class SocketError : public std::runtime_error {
public:
    SocketError(std::string msg) : runtime_error((SOCKET + msg).c_str()) {}
};

class Socket {
private:
    int sockfd;
    SockaddrIn bindsin, cnctsin;

public:
    Socket(int type);
    ~Socket();
    void setOpt(int option);
    void bindSock(in_addr_t ip, int port);
    void connectTo(in_addr_t ip, int port);
    void closeSock();
    
    Address addr();
    int fd();
};

#endif