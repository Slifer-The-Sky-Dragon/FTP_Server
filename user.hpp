#ifndef _USER_HPP
#define _USER_HPP

#include <string>

using namespace std;

#define USER_FIELDS_NUM 4

#define USERNAME_FIELD "user"
#define PASSWORD_FIELD "password"
#define ADMIN_FILED "admin"
#define DOWNLOAD_SIZE_FIELD "size"

struct User{
	string username;
	string password;
	bool is_admin;
	int download_size;
};

#endif