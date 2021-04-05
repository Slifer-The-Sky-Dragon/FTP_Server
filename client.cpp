#include <iostream>
#include <utility>
#include <cstring>
#include <sys/select.h>
#include "socket.hpp"

using namespace std;
const int BUF_SZ = 1000;
const int SRVR_CMD_PORT = 8000;
const int SRVR_DATA_PORT = 8001;
const int PORT_OFFSET = 10000;

void find_usable_ports(Socket& cmd_sock, Socket& data_sock) {
    for (int i = PORT_OFFSET; i < PORT_OFFSET + 50; i += 2) {
        try {
            cmd_sock.bindSock(INADDR_LOOPBACK, i);
            data_sock.bindSock(INADDR_LOOPBACK, i + 1);
            cout << "settled at " << i << '\n';
            return;
        } catch (SocketError& e) {
            cout << "i=" << i << ": " << e.what() << '\n';
        }
    }

    throw runtime_error("No free port pair found");
}

void check_srvr_resp(int fd, fd_set* readfds) {
    if (FD_ISSET(fd, readfds)) {
        cout << "New event (fd=" << fd << ")" << '\n';
        char buf[BUF_SZ];
        memset(buf, 0, BUF_SZ);
        int res = recv(fd, buf, BUF_SZ, 0);
        if (res < 0) {
            perror("read");
            exit(EXIT_SUCCESS);
        }

        if (res == 0) {
            cout << "Descriptor closed unexpectedly" << '\n';
            exit(EXIT_FAILURE);
        }
        cout << "recieved message: " << buf;
    }
}

void check_stdin(int cmd_sock, fd_set* readfds) {
    if (FD_ISSET(STDIN_FILENO, readfds)) {
        char buf[BUF_SZ];
        memset(buf, 0, BUF_SZ);
        read(STDIN_FILENO, buf, BUF_SZ);

        int res = send(cmd_sock, buf, strlen(buf), 0);
        if (res < 0)
            perror("send");
    }
}

int make_reading_list(fd_set* readfds, int cmd_sock, int data_sock) {
    FD_ZERO(readfds);
	FD_SET(cmd_sock, readfds);
	FD_SET(data_sock, readfds);
    FD_SET(STDIN_FILENO, readfds);
    return max(cmd_sock, data_sock);
}

int main() {
    Socket cmd_sock(SOCK_STREAM);
    Socket data_sock(SOCK_STREAM);

    cout << "sockets created (fd: cmd, data= " << cmd_sock.fd()
        << ", " << data_sock.fd() << ")" << '\n';

    find_usable_ports(cmd_sock, data_sock);

    cmd_sock.connectTo(INADDR_LOOPBACK, SRVR_CMD_PORT);
    data_sock.connectTo(INADDR_LOOPBACK, SRVR_DATA_PORT);

    cout << "looks fine to me" << '\n';
    fd_set readfds;

    while (true) {
        int max_fd = make_reading_list(&readfds, cmd_sock.fd(), data_sock.fd());
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0)
            perror("select");
        
        check_srvr_resp(cmd_sock.fd(), &readfds);
        check_srvr_resp(data_sock.fd(), &readfds);
        check_stdin(cmd_sock.fd(), &readfds);
    }
    
    cmd_sock.closeSock();
    data_sock.closeSock();
}