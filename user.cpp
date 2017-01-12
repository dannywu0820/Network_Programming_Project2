#include"user.h" //remember to include yourself

#include<iostream>
#include<sstream>
#include<string>
using namespace std;

User::User(int sockfd, const char* ip, int port){
	stringstream ss;
	ss << ip << "/" <<port;
	
	this->sockfd = sockfd;
	this->nickname = "(no name)";
	this->ip_port = ss.str();
}

int User::get_sockfd(){
	return this->sockfd;
}

string User::get_nickname(){
	return this->nickname;
}

string User::get_ip_port(){
	return this->ip_port;
}