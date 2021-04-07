#include <iostream>
#include <utility>
#include <cstring>
#include <sstream>
#include <vector>
#include <sys/select.h>
#include "socket.hpp"

std::vector<std::string> tokenize(std::string line, char seperator) {
    int start = 0;
    std::string token;
    std::vector<std::string> tokens;
    for (size_t i = 0; i < line.length(); ++i) {
        if (i == line.length() - 1 && line[i] != seperator) {
            token = line.substr(start, i - start + 1);
            tokens.push_back(token);
        }
        else {
            if(line[i] == seperator){
            token = line.substr(start, i - start);
            start = i + 1;
            tokens.push_back(token);
            }
        }
    }
    return tokens;
}

using namespace std;

typedef vector<string> Command;

const int MAX_MESSAGE_LEN = 512;
const int SRVR_CMD_PORT = 8000;
const int SRVR_DATA_PORT = 8001;
const int PORT_OFFSET = 10000;

void find_usable_ports(Socket& cmd_sock, Socket& data_sock) {
    for (int i = PORT_OFFSET; i < (1 << 16); i += 2) {
        try {
            cmd_sock.bindSock(INADDR_LOOPBACK, i);
            data_sock.bindSock(INADDR_LOOPBACK, i + 1);
            cout << "sockets bound successfully (port: cmd, data="
                << i << ", " << i + 1 << ")" << '\n';
            return;
        } catch (SocketError& e) {}
    }

    throw runtime_error("No free port pair found");
}

void check_srvr_cmd_resp(int fd, fd_set* readfds) {
    if (FD_ISSET(fd, readfds)) {
        cout << "New event (fd=" << fd << ")" << '\n';
        char buf[MAX_MESSAGE_LEN];
        memset(buf, 0, MAX_MESSAGE_LEN);
        int res = recv(fd, buf, MAX_MESSAGE_LEN, 0);
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

void check_srvr_data_resp(int fd, fd_set* readfds, const Command last_cmd) {
    char buf[MAX_MESSAGE_LEN];

    if (FD_ISSET(fd, readfds)) {
        cout << "New event (fd=" << fd << ")" << '\n';
        memset(buf, 0, MAX_MESSAGE_LEN);
        int res = recv(fd, buf, MAX_MESSAGE_LEN, 0);
        if (res < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        if (res == 0) {
            cout << "Descriptor closed unexpectedly" << '\n';
            exit(EXIT_FAILURE);
        }

        if (last_cmd[0] != "retr")
            cout << "recieved message: " << buf << '\n';
    }
}

void check_stdin(int cmd_sock, fd_set* readfds, Command& last_cmd) {
    if (FD_ISSET(STDIN_FILENO, readfds)) {
        char buf[MAX_MESSAGE_LEN];

        memset(buf, 0, MAX_MESSAGE_LEN);
        read(STDIN_FILENO, buf, MAX_MESSAGE_LEN);

        int res = send(cmd_sock, buf, strlen(buf), 0);
        if (res < 0)
            perror("send");
        last_cmd = tokenize(string(buf), ' ');
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

    cout << "connected to server, ready to go!" << '\n';
    fd_set readfds;
    Command last_cmd;

    while (true) {
        int max_fd = make_reading_list(&readfds, cmd_sock.fd(), data_sock.fd());
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0)
            perror("select");
        
        check_srvr_cmd_resp(cmd_sock.fd(), &readfds);
        check_srvr_data_resp(data_sock.fd(), &readfds, last_cmd);
        check_stdin(cmd_sock.fd(), &readfds, last_cmd);
    }
    
    cmd_sock.closeSock();
    data_sock.closeSock();
}