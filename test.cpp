#include <iostream>
// #include "parser.hpp"
// #include "user.hpp"
#include <vector>

#include "socket.hpp"

using namespace std;

int main(){
	// Json_parser my_parser("config.json");
	// cout << my_parser.get_raw_data() << endl;
// 	cout << my_parser.get_server_command_port() << endl;
// 	cout << my_parser.get_server_data_port() << endl;

// 	vector < User > users = my_parser.get_server_users();

// 	for(int i = 0 ; i < users.size() ; i++)
// 		cout << users[i].username << ' ' << users[i].password << ' ' << users[i].is_admin << ' ' << users[i].download_size << endl;

    Socket s(SOCK_STREAM);
    s.bindSock(INADDR_LOOPBACK, 10000);
    s.setOpt(SO_REUSEADDR);
    cout << "socket created and bound successfully" << '\n';
    cout << "fd: " << s.fd() << ", addr: " << s.addr().first << ", " << s.addr().second << '\n';
}