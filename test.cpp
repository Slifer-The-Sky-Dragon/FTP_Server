#include <iostream>
// #include "parser.hpp"
// #include "user.hpp"
#include <vector>
#include "user.hpp"
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

	int server_command_port = 8000;
	int server_data_port = 8001;

	User admin;
	admin.username = "Ali";
	admin.password = "1234";
	admin.is_admin = 1;
	admin.download_size = 100000;

    Socket s(SOCK_STREAM);
    s.bindSock(INADDR_LOOPBACK, 10000);
    s.setOpt(SO_REUSEADDR);
    cout << "socket created and bound successfully" << '\n';
    cout << "fd: " << s.fd() << ", addr: " << s.addr().first << ", " << s.addr().second << '\n';
}


