#include <iostream>
#include <utility>
#include "socket.hpp"

using namespace std;

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

int main() {
    Socket cmd_sock(SOCK_STREAM);
    Socket data_sock(SOCK_STREAM);

    find_usable_ports(cmd_sock, data_sock);

    cmd_sock.connectTo(INADDR_LOOPBACK, SRVR_CMD_PORT);
    data_sock.connectTo(INADDR_LOOPBACK, SRVR_DATA_PORT);

    cout << "looks fine to me" << '\n';
    sleep(60);
    
    cmd_sock.closeSock();
    data_sock.closeSock();
}