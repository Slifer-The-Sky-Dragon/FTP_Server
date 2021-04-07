#include <iostream>
#include <algorithm>
#include <vector>
#include <cstring>
#include <map>
#include <sstream>
#include <unistd.h>
#include <dirent.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include "jute.h"
#include "user.hpp"
#include "socket.hpp"

using namespace std;

#define COMMAND_COMPLETED 0

#define CONFIG_FILE_NAME "config.json"

#define COMMAND_PORT_JSON_FIELD "commandChannelPort"
#define DATA_PORT_JSON_FIELD "dataChannelPort"
#define USERS_JSON_FIELD "users"
#define FILES_JSON_FIELD "files"

#define JSON_USER_USERNAME_FIELD "user"
#define JSON_USER_PASSWORD_FIELD "password"
#define JSON_USER_ADMIN_FIELD "admin"
#define JSON_USER_SIZE_FIELD "size"

#define BASE_STATE 0
#define MID_STATE 1
#define USER_STATE 2
#define ADMIN_STATE 3

#define EMPTY ""
#define MAX_MESSAGE_LEN (1 << 14)

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
#define SUCCESSFUL_CHANGE "250: Successful change.\n"
#define SUCCESSFUL_DOWNLOAD "226: Successful download.\n"
#define STX_ERROR "501: Syntax error in parameters or arguments.\n"
#define FILE_UNAVAILABLE "520: File unavailable.\n"
#define LIST_TRANSFER_DONE "206: List transfer done.\n"
#define DEL_FILE "-f"
#define DEL_DIR "-d"

#define DOWNLOAD_ACC string("download acc\n")
#define DOWNLOAD_REJ string("download rej\n")

typedef int Port;
typedef int Sfd;
typedef int State;
typedef string Username;
typedef string Directory;
typedef string File;

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

string abspath(string path) {
    char buf[MAX_MESSAGE_LEN];
    memset(buf, 0, MAX_MESSAGE_LEN);
    if (!realpath(path.c_str(), buf))
        perror("realpath");
    return string(buf);
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

bool check_login(int client_sockfd, map<Sfd, Login_State>& clients_state) {
    if(clients_state[client_sockfd].login_state == BASE_STATE || 
                    clients_state[client_sockfd].login_state == MID_STATE)
        return false;
    
    return true;
}

bool check_admin(int client_sockfd, map<Sfd, Login_State>& clients_state) {
    return (clients_state[client_sockfd].login_state == ADMIN_STATE);
}

bool file_is_restricted(string full_path, vector<File>& admin_files) {
    for (auto filename: admin_files) {
        string full_filename = abspath(clear_new_line(BASE_DIR) + "/" + filename);
        if (full_path == full_filename)
            return true;
    }
    return false;
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
        return "257: " + new_dir + " created\n";
    return ERROR;
}

string delete_command_handler(int client_sockfd, stringstream& command_stream,
                map<Sfd, Login_State>& clients_state, vector<File>& admin_files){
    if (!check_login(client_sockfd, clients_state))
        return NEED_ACC;
    
    string flag, path;
    command_stream >> flag >> path;

    string composed_path = clear_new_line(clients_state[client_sockfd].cur_dir) + "/" + path;
    string full_path = abspath(composed_path);

    if (!check_admin(client_sockfd, clients_state) && file_is_restricted(full_path, admin_files))
        return FILE_UNAVAILABLE;
    
    if (flag == DEL_FILE) {
        if (unlink(full_path.c_str()) == -1) {
            perror("unlink");
            return ERROR;
        }
    }
    else if (flag == DEL_DIR) {
        if (rmdir(full_path.c_str()) == -1) {
            perror("rmdir");
            return ERROR;
        }
    }
    else {
        cout << "delete: undefined flag" << '\n';
        return STX_ERROR;
    }

    string log = "250: " + path + " deleted.\n";
    return log;
}

string rename_command_handler(int client_sockfd, stringstream& command_stream,
                map<Sfd, Login_State>& clients_state, vector<File>& admin_files){
    if (!check_login(client_sockfd, clients_state))
        return NEED_ACC;
    
    string from, to;
    command_stream >> from >> to;

    string composed_from = clear_new_line(clients_state[client_sockfd].cur_dir) + "/" + from;
    string composed_to = clear_new_line(clients_state[client_sockfd].cur_dir) + "/" + to;

    string abs_from = abspath(composed_from);
    string abs_to = abspath(composed_to);

    cout << "abs_from: " << abs_from << '\n';
    cout << "abs_to: " << abs_to << '\n';

    if (!check_admin(client_sockfd, clients_state) && file_is_restricted(abs_from, admin_files))
        return FILE_UNAVAILABLE;

    if (rename(abs_from.c_str(), abs_to.c_str()) == -1) {
        perror("rename");
        return ERROR;
    }

    return SUCCESSFUL_CHANGE;
}

string chdir_command_handler(int client_sockfd, stringstream& command_stream,
                                map<Sfd, Login_State>& clients_state){
    if (!check_login(client_sockfd, clients_state))
        return NEED_ACC;

    string path;
    command_stream >> path;

    if(path == EMPTY){
        clients_state[client_sockfd].cur_dir = BASE_DIR;
        return SUCCESSFUL_CHANGE;
    }

    string composed_path = clear_new_line(clients_state[client_sockfd].cur_dir) + "/" + path;
    string abs_path = abspath(composed_path);

    if (access(abs_path.c_str(), F_OK) == -1) {
        perror("access");
        return STX_ERROR;
    }

    clients_state[client_sockfd].cur_dir = abs_path + "\n";
    return SUCCESSFUL_CHANGE;
}

vector<string> list_dir(string full_path) {
    vector<string> res;
    DIR* dir = opendir(clear_new_line(full_path).c_str());
    struct dirent* dent; 
    if (dir == NULL) {
        perror("opendir");
        return res;
    }

    while ((dent = readdir(dir)) != NULL) {
        res.push_back(string(dent->d_name));
    }

    return res;
}

string ls_command_handler(int client_sockfd, stringstream& command_stream,
                map<Sfd, Login_State>& clients_state, map<int, int>& command_fd_to_data_fd){
    if (!check_login(client_sockfd, clients_state))
        return NEED_ACC;

    auto dir_files = list_dir(clients_state[client_sockfd].cur_dir);
    string ls_res = "";
    for (auto f : dir_files)
        ls_res += f + " ";
    ls_res += "\n";
    cout << "ls_res: " << ls_res << '\n';

    int client_data_sockfd = command_fd_to_data_fd[client_sockfd];
    send_response_to_client(client_data_sockfd, ls_res);
    return LIST_TRANSFER_DONE;
}

string upload_file(int client_sockfd, string full_path,
    map <Sfd, Login_State>& clients_state, map<int, int>& command_fd_to_data_fd) {
    // send_response_to_client(client_data_sock_fd, DOWNLOAD_ACC);
    int file_fd = open(full_path.c_str(), O_RDONLY);
    if (file_fd == -1)
        perror("open");
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t offset = 0;
    int data_sockfd = command_fd_to_data_fd[client_sockfd];
    int res = sendfile(data_sockfd, file_fd, &offset, file_stat.st_size);
    if (res < file_stat.st_size)
        perror("sendfile");

    return SUCCESSFUL_DOWNLOAD;
}

string download_command_handler(int client_sockfd, stringstream& command_stream,
                map<Sfd, Login_State>& clients_state, map<int, int>& command_fd_to_data_fd,
                vector<File>& admin_files){
    int client_data_sockfd = command_fd_to_data_fd[client_sockfd];
    
    if (!check_login(client_sockfd, clients_state)) {
        // send_response_to_client(client_data_sockfd, DOWNLOAD_REJ);
        return NEED_ACC;
    }
    string path;
    command_stream >> path;
    string composed_path = clear_new_line(clients_state[client_sockfd].cur_dir) + "/" + path;
    string full_path = abspath(composed_path);

    if (!check_admin(client_sockfd, clients_state) && file_is_restricted(full_path, admin_files)) {
        // send_response_to_client(client_data_sockfd, DOWNLOAD_REJ);
        return FILE_UNAVAILABLE;
    }
    if (access(full_path.c_str(), F_OK) == -1) {
        // send_response_to_client(client_data_sockfd, DOWNLOAD_REJ);
        return ERROR;
    }
    string log = upload_file(client_sockfd, full_path, clients_state, command_fd_to_data_fd);
    return log;
}

string command_handler(string command_message , int client_sockfd , map < int , int >& command_fd_to_data_fd , 
                                         map < Sfd , Login_State >& clients_state , 
                                         vector < User > users, vector < File >& admin_files){
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
    else if(command == "dele")
        log = delete_command_handler(client_sockfd , command_stream , clients_state, admin_files);
    else if(command == "rename")
        log = rename_command_handler(client_sockfd, command_stream, clients_state, admin_files);
    else if(command == "cwd")
        log = chdir_command_handler(client_sockfd, command_stream, clients_state);
    else if(command == "ls")
        log = ls_command_handler(client_sockfd, command_stream, clients_state, command_fd_to_data_fd);
    else if(command == "retr")
        log = download_command_handler(client_sockfd, command_stream, clients_state,
            command_fd_to_data_fd, admin_files);
    else{
        if(clients_state[client_sockfd].login_state == MID_STATE){
            clients_state[client_sockfd].login_state = BASE_STATE;
            log = BAD_SEQUENCE_OF_COMMANDS;
        }

        else
            log = ERROR;
    }

    send_response_to_client(client_sockfd , log);
    return log;
}

void check_for_clients_responses(fd_set* read_sockfd_list , Client_info client_sockfd_list[] , char* result , 
                                                map < int , int >& command_fd_to_data_fd , 
                                                map < Sfd , Login_State >& clients_state , 
                                                vector < User > users, vector < File >& admin_files){
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
                                                clients_state , users , admin_files);
                cout << log << endl;
            }
        }
    }
}

string read_data_from_file(string file_name){
    string result = EMPTY;

    ifstream file_stream(file_name);

    string cur_line;
    while(getline(file_stream , cur_line)){
        result += cur_line;
    }
    return result;
}

void initial_server_variables_from_config_file(string config_file_name , int& server_command_port , 
                                            int& server_data_port , vector < User >& users , 
                                            vector < File >& admin_files){
    string data = read_data_from_file(config_file_name);
    jute::jValue jparser = jute::parser::parse(data);

    server_command_port = jparser[COMMAND_PORT_JSON_FIELD].as_int();
    server_data_port = jparser[DATA_PORT_JSON_FIELD].as_int();

    for(int i = 0 ; i < jparser[USERS_JSON_FIELD].size() ; i++){
        User new_user;
        new_user.username = jparser[USERS_JSON_FIELD][i][JSON_USER_USERNAME_FIELD].as_string();
        new_user.password = jparser[USERS_JSON_FIELD][i][JSON_USER_PASSWORD_FIELD].as_string();
        new_user.is_admin = (jparser[USERS_JSON_FIELD][i][JSON_USER_ADMIN_FIELD].as_string() == "true");
        new_user.download_size = jparser[USERS_JSON_FIELD][i][JSON_USER_SIZE_FIELD].as_int();
        users.push_back(new_user);    
    }

    for(int i = 0 ; i < jparser[FILES_JSON_FIELD].size() ; i++){
        File new_file = jparser[FILES_JSON_FIELD][i].as_string();
        admin_files.push_back(new_file);
    }

}

int main(){

    Client_info client_sockfd_list[MAX_USER];

    map < int , int > command_fd_to_data_fd;
    map < Sfd , Login_State > clients_state;
    vector < User > users;
    vector < File > admin_files;

    BASE_DIR = getcwd(NULL , 0);
    BASE_DIR += "\n";

    int server_command_port = 8000;
    int server_data_port = 8001;

    initial_server_variables_from_config_file(CONFIG_FILE_NAME , server_command_port , 
                                                server_data_port , users , admin_files);

    //Server information
        cout << "----> Command port = " << server_command_port << endl;
        cout << "----> Data port = " << server_data_port << endl;
        cout << "----> Users : " << endl;
        for(int i = 0 ; i < users.size() ; i++){
            cout << users[i].username << " - " << users[i].password << " - " << users[i].is_admin << " - ";
            cout << users[i].download_size << endl;
        }
        cout << "----> Files: " << endl;
        for(int i = 0 ; i < admin_files.size() ; i++){
            cout << admin_files[i] << endl;
        }
    //

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
                                                    clients_state , users, admin_files);
    }

}