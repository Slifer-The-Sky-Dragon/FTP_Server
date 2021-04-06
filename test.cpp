#include <iostream>
#include <algorithm>
#include <vector>
#include "user.hpp"
#include "socket.hpp"
#include <map>
#include <sstream>
#include <unistd.h>

using namespace std;

#define COMMAND_COMPLETED 0

#define BASE_STATE 0
#define MID_STATE 1
#define USER_STATE 2
#define ADMIN_STATE 3

#define EMPTY ""
#define MAX_MESSAGE_LEN 100

#define port first
#define sfd second

#define MAX_USER 1000
#define NO_USER -1
#define MAX_PENDING_CON 5
#define COMMAND 1
#define DATA 2

#define BAD_SEQUENCE_OF_COMMANDS "503: Bad sequence of commands.\n"
#define USERNAME_IS_OK "331: User name okay, need password.\n"
#define INVALID_USER_PASS "430: Invalid username or password.\n"
#define SUCCESSFUL_LOGIN "230: User logged in, proceed. Logged out if appropriate.\n"
#define ERROR "500: Error\n"
#define SUCCESSFUL_QUIT "221: Successful Quit.\n"
#define NEED_ACC "332: Need account for login.\n"

typedef int Port;
typedef int Sfd;
typedef int State;
typedef string Username;
typedef string Directory;

typedef pair < Port , Sfd > Client_info;

string BASE_DIR;

struct Login_State{
	State login_state;
	Username login_username;
	Directory cur_dir;
};

string convert_to_string(char* str , int length){
	string result = EMPTY;
	for(int i = 0 ; i < length ; i++){
		result += str[i];
	}
	return result;
}

string clear_new_line(string x){
	string result = EMPTY;
	for(int i = 0 ; i < x.size() ; i++){
		if(x[i] != '\n')
			result += x[i];
	}
	return result;
}

void fill_string_zero(char* str){
	for(int i = 0 ; i < MAX_MESSAGE_LEN ; i++)
		str[i] = '\0';
}

void send_response_to_client(int client_sockfd , string message){
	if(send(client_sockfd , message.c_str() , MAX_MESSAGE_LEN , 0) != MAX_MESSAGE_LEN)
		cout << "Failed to send message...\n";
	else
		cout << "Message sent succesfully!\n";	
}

void send_response_to_client(int client_sockfd , char* message){
	if(send(client_sockfd , message , MAX_MESSAGE_LEN , 0) != MAX_MESSAGE_LEN)
		cout << "Failed to send message..." << endl;
	else
		cout << "Message sent succesfully!" << endl;	
}

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

void check_for_new_connection(int command_or_data , int acceptor_sockfd , fd_set* read_sockfd_list ,
									 Client_info client_sockfd_list[] , map < int , int >& command_fd_to_data_fd ,
									 map < Sfd , Login_State >& clients_state){
	int client_sockfd;
	struct sockaddr_in client_address;

	if(FD_ISSET(acceptor_sockfd , read_sockfd_list)){
		socklen_t clsize = sizeof(client_address);

		if((client_sockfd = accept(acceptor_sockfd , (struct sockaddr *) &client_address , &clsize)) < 0)
			cout << "Connection accept error...\n";
		else{
			cout << "New client has joined...\n";

			int client_port = ntohs(client_address.sin_port);

			for(int i = 0 ; i < MAX_USER ; i++){
				if(client_sockfd_list[i].sfd == NO_USER){
					client_sockfd_list[i].sfd = client_sockfd;
					client_sockfd_list[i].port = client_port;
					break;
				}
			}

			if(command_or_data == DATA){
				for(int i = 0 ; i < MAX_USER ; i++){
					if(client_sockfd_list[i].port == client_port - 1){
						command_fd_to_data_fd[client_sockfd_list[i].sfd] = client_sockfd;
						cout << "Command and Data fetched..." << endl;
						break;
					}
				}
			}
			else{
				clients_state[client_sockfd].login_state = BASE_STATE;
				clients_state[client_sockfd].login_username = EMPTY;
				clients_state[client_sockfd].cur_dir = BASE_DIR;
			}
		}
	}
}

string username_command_handler(int client_sockfd , stringstream& command_stream ,
								map < Sfd , Login_State >& clients_state){

	string client_username;
	command_stream >> client_username;

	if(clients_state[client_sockfd].login_state != BASE_STATE){
		return BAD_SEQUENCE_OF_COMMANDS;
	}

	clients_state[client_sockfd].login_state = MID_STATE;
	clients_state[client_sockfd].login_username = client_username;

	return USERNAME_IS_OK;
}

int check_username_and_password(string username, string password, vector < User > users){
	for(int i = 0 ; i < users.size() ; i++){
		if(users[i].username == username){
			if(users[i].password == password){
				if(users[i].is_admin == true)
					return ADMIN_STATE;
				else
					return USER_STATE;
			}
			else{
				return BASE_STATE;
			}
		}
	}
	return BASE_STATE;
}

string password_command_handler(int client_sockfd , stringstream& command_stream ,
								map < Sfd , Login_State >& clients_state , 
								vector < User > users){

	string client_password;
	command_stream >> client_password;

	if(clients_state[client_sockfd].login_state != MID_STATE){
		return BAD_SEQUENCE_OF_COMMANDS;
	}

	int login_status = check_username_and_password(clients_state[client_sockfd].login_username, client_password, users);

	if(login_status == BASE_STATE){
		clients_state[client_sockfd].login_state = BASE_STATE;
		clients_state[client_sockfd].login_username = EMPTY;
		return INVALID_USER_PASS;
	}
	else{
		clients_state[client_sockfd].login_state = login_status;
		return SUCCESSFUL_LOGIN;
	}
}

string quit_command_handler(int client_sockfd , map < Sfd , Login_State >& clients_state){
	if(clients_state[client_sockfd].login_state == BASE_STATE || 
						clients_state[client_sockfd].login_state == MID_STATE)
		return NEED_ACC;

	clients_state[client_sockfd].login_state = BASE_STATE;
	clients_state[client_sockfd].cur_dir = BASE_DIR;

	return SUCCESSFUL_QUIT;
}

string pwd_command_handler(int client_sockfd , map < Sfd , Login_State >& clients_state){
	if(clients_state[client_sockfd].login_state == BASE_STATE || 
						clients_state[client_sockfd].login_state == MID_STATE)
		return NEED_ACC;

	return clients_state[client_sockfd].cur_dir;
}

string mkd_command_handler(int client_sockfd , stringstream& command_stream ,
							map < Sfd , Login_State >& clients_state){
	if(clients_state[client_sockfd].login_state == BASE_STATE || 
						clients_state[client_sockfd].login_state == MID_STATE)
		return NEED_ACC;

	string new_dir;
	command_stream >> new_dir;
	string tcommand = "mkdir ";
	tcommand += clear_new_line(clients_state[client_sockfd].cur_dir) + "/" + new_dir;
	if(system(tcommand.c_str()) == COMMAND_COMPLETED)
		return "257: " + new_dir + " created";
	return ERROR;
}



string command_handler(string command_message , int client_sockfd , map < int , int >& command_fd_to_data_fd , 
										 map < Sfd , Login_State >& clients_state , 
										 vector < User > users){
	string log = "Nothing...\n";

	stringstream command_stream(command_message);

	string command;
	command_stream >> command;

	if(command == "user")
		log = username_command_handler(client_sockfd , command_stream , clients_state);
	else if(command == "pass")
		log = password_command_handler(client_sockfd , command_stream , clients_state , users);
	else if(command == "quit")
		log = quit_command_handler(client_sockfd , clients_state);
	else if(command == "pwd")
		log = pwd_command_handler(client_sockfd , clients_state);
	else if(command == "mkd")
		log = mkd_command_handler(client_sockfd , command_stream , clients_state);
	else{
		if(clients_state[client_sockfd].login_state == MID_STATE){
			clients_state[client_sockfd].login_state = BASE_STATE;
			log = BAD_SEQUENCE_OF_COMMANDS;
		}
	}

	send_response_to_client(command_fd_to_data_fd[client_sockfd] , log);
	return log;
}

void check_for_clients_responses(fd_set* read_sockfd_list , Client_info client_sockfd_list[] , char* result , 
												map < int , int >& command_fd_to_data_fd , 
												map < Sfd , Login_State >& clients_state , 
												vector < User > users){
	for(int i = 0 ; i < MAX_USER ; i++){
		if(client_sockfd_list[i].sfd == NO_USER)
			continue;

		if(FD_ISSET(client_sockfd_list[i].sfd , read_sockfd_list)){
			fill_string_zero(result);
			int message_len = recv(client_sockfd_list[i].sfd , result , MAX_MESSAGE_LEN , 0);

			string received_message = convert_to_string(result , message_len);
			if(message_len < 0)
				cout << "Recieving error..." << endl;
			else if(message_len == 0){
				cout << "A client has diconnected from server! Closing socket..." << endl;
				close(client_sockfd_list[i].sfd);
				client_sockfd_list[i].sfd = NO_USER;
				client_sockfd_list[i].port = NO_USER;
			}
			else{
				string log = command_handler(received_message , client_sockfd_list[i].sfd , command_fd_to_data_fd , 
												clients_state , users);
				cout << log << endl;
			}
		}
	}
}

int main(){

	Client_info client_sockfd_list[MAX_USER];

	map < int , int > command_fd_to_data_fd;
	map < Sfd , Login_State > clients_state;

	BASE_DIR = getcwd(NULL , 0);
	BASE_DIR += "\n";
	// Json_parser my_parser("config.json");
	// cout << my_parser.get_raw_data() << endl;
// 	cout << my_parser.get_server_command_port() << endl;
// 	cout << my_parser.get_server_data_port() << endl;

// 	vector < User > users = my_parser.get_server_users();

// 	for(int i = 0 ; i < users.size() ; i++)
// 		cout << users[i].username << ' ' << users[i].password << ' ' << users[i].is_admin << ' ' << users[i].download_size << endl;

	int server_command_port = 8000;
	int server_data_port = 8001;

	User test_user;
	test_user.username = "Ali";
	test_user.password = "1234";
	test_user.is_admin = 1;
	test_user.download_size = 100000;

	vector < User > users;
	users.push_back(test_user);

	fd_set read_sockfd_list;

	char* result = (char*) malloc(MAX_MESSAGE_LEN * sizeof(char));
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
		
		check_for_new_connection(COMMAND , command_sock.fd() , &read_sockfd_list , 
													client_sockfd_list , command_fd_to_data_fd , clients_state);
		check_for_new_connection(DATA , data_sock.fd() , &read_sockfd_list , 
													client_sockfd_list , command_fd_to_data_fd , clients_state);
		check_for_clients_responses(&read_sockfd_list , client_sockfd_list , result , command_fd_to_data_fd ,
													clients_state , users);
	}

}


