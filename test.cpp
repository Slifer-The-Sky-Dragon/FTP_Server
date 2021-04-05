#include <iostream>
#include <algorithm>
#include <vector>
#include "user.hpp"
#include "socket.hpp"
#include <map>

using namespace std;

#define port first
#define sfd second
#define MAX_USER 1000
#define NO_USER -1
#define MAX_PENDING_CON 5
#define COMMAND 1
#define DATA 2

typedef int Port;
typedef int Sfd;
typedef pair < Port , Sfd > Client_info;

int make_reading_list(fd_set* read_sockfd_list , int command_sock , int data_sock , Client_info client_sockfd_list[]){
	FD_ZERO(read_sockfd_list);
	FD_SET(command_sock , read_sockfd_list);
	FD_SET(data_sock , read_sockfd_list);
	
	int max_fd = max(command_sock , data_sock);

	for(int i = 0 ; i < MAX_USER ; i++){
		if(client_sockfd_list[i].sfd != NO_USER){
			FD_SET(client_sockfd_list[i].sfd , read_sockfd_list);
		}
		if(max_fd < client_sockfd_list[i].sfd)
			max_fd = client_sockfd_list[i].sfd;
	}	

	return max_fd;
}

void check_for_new_connection(int command_or_data , int acceptor_sockfd , 
									fd_set* read_sockfd_list , Client_info client_sockfd_list[]){
	int client_sockfd;
	struct sockaddr_in client_address;

	if(FD_ISSET(acceptor_sockfd , read_sockfd_list)){
		socklen_t clsize = sizeof(client_address);

		if((client_sockfd = accept(acceptor_sockfd , (struct sockaddr *) &client_address , &clsize)) < 0)
			cout << "Connection accept error...\n";
		else{
			cout << "New client has joined...\n";

			//send_response_to_client(client_sockfd , "Welcome!");

			for(int i = 0 ; i < MAX_USER ; i++){
				if(client_sockfd_list[i].sfd == NO_USER){
					client_sockfd_list[i].sfd = client_sockfd;
					client_sockfd_list[i].port = ntohs(client_address.sin_port);
					break;
				}
			}
		}
	}
}


int main(){

	Client_info client_sockfd_list[MAX_USER];
	map < int , int > command_fd_to_data_fd;
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

	fd_set read_sockfd_list;

	//Command Socket
    Socket command_sock(SOCK_STREAM);
    command_sock.setOpt(SO_REUSEADDR);
    command_sock.bindSock(INADDR_LOOPBACK, server_command_port);

    //Data Socket
    Socket data_sock(SOCK_STREAM);
    data_sock.setOpt(SO_REUSEADDR);
    data_sock.bindSock(INADDR_LOOPBACK, server_data_port);

	if(listen(command_sock.fd() , MAX_PENDING_CON) < 0)
		cout << "Command Socket Listening error..." << endl;
	if(listen(data_sock.fd() , MAX_PENDING_CON) < 0)
		cout << "Data Socket Listening error..." << endl;

	//initializing client list
	for(int i = 0 ; i < MAX_USER ; i++){
		client_sockfd_list[i].port = NO_USER;
		client_sockfd_list[i].sfd = NO_USER;
	}

	//communicate with clients
	while(1){
		int max_fd = make_reading_list(&read_sockfd_list , command_sock.fd() , data_sock.fd() , client_sockfd_list);

		int activity = select(max_fd + 1 , &read_sockfd_list , NULL , NULL , NULL);
		if(activity < 0)
			cout << "Select Error..." << endl;
		
		check_for_new_connection(COMMAND , command_sock.fd() , &read_sockfd_list , client_sockfd_list);
		check_for_new_connection(DATA , data_sock.fd() , &read_sockfd_list , client_sockfd_list);
		// check_for_clients_responses(&read_sockfd_list , client_sockfd_list , group , result , server_port);
	}

}