#include<iostream>
#include<string>
using namespace std;

#ifndef USER_H
#define USER_H

class User{
	private:
		int sockfd;
		string nickname;
		string ip_port;
	public:
		User(int sockfd, const char* ip, int port);
		int get_sockfd();
		string get_nickname();
		string get_ip_port();
};

#endif