#include"command.h"
#include"user.h" 

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#include<time.h>
#include<sys/signal.h>
#include<sys/wait.h>
#include<sys/errno.h>
#include<sys/resource.h>
#include<stdarg.h>
#include<fcntl.h>

#include<iostream>
#include<sstream>
#include<string>
#include<vector>
using namespace std;

void Command::who(vector<User> user_table, int sockfd){ //who
	stringstream result;
	vector<User>::iterator it;
	
	result << "<ID>[Tab]<nickname>[Tab]<IP/port>[Tab]<indicate me>\n";
	for(it = user_table.begin(); it != user_table.end(); it++){
		result << it->get_sockfd() << "	" << it->get_nickname() << "	" << it->get_ip_port();
		if(it->get_sockfd() == sockfd) result << "	<-me\n";
		else result << "	    \n";
	}
	
	cout << result.str();
}
void Command::name(vector<User>* user_table, int sockfd, char* name){ //name (name)
	stringstream result;
	vector<User>::iterator it, user_now;
	bool repeated = false;
			
	for(it = user_table->begin(); it != user_table->end(); it++){
		if(it->get_sockfd() == sockfd){
			user_now = it;
		}
		else{
			if(it->get_nickname().compare(name) == 0) repeated = true;
		}
	}
			
	if(!repeated){
		user_now->set_nickname(name);
		result << "*** User from " << user_now->get_ip_port() << " is named '" << user_now->get_nickname() << "' ***\n";
		
		for(it = user_table->begin(); it != user_table->end(); it++){
			write(it->get_sockfd(), result.str().c_str(), result.str().length());
		}
	}
	else{
		result << "*** User '" << name << "' already exists. ***\n";
		
		write(user_now->get_sockfd(), result.str().c_str(), result.str().length());
	}
}
void Command::tell(vector<User> user_table, int sockfd, char* message, int recv_sockfd){ //tell (client id) (message)
	stringstream result;
	vector<User>::iterator it, user_now;
	bool existed = false;
	for(it = user_table.begin(); it != user_table.end(); it++){
		if(it->get_sockfd() == sockfd){
			user_now = it;
		}
		if(it->get_sockfd() == recv_sockfd){
			existed = true;
		}
	}
			
	if(existed){
		result << "*** " << user_now->get_nickname() << " told you ***: " << message;
		write(recv_sockfd, result.str().c_str(), result.str().length());
	}
	else{
		result << "*** Error: user #" << recv_sockfd << " does not exist yet. ***\n";
		write(user_now->get_sockfd(), result.str().c_str(), result.str().length());
	}
}
void Command::yell(vector<User> user_table, int sockfd, char* message){ //yell (message)
	stringstream result;
	vector<User>::iterator it, user_now;
	
	for(it = user_table.begin(); it != user_table.end(); it++){
		if(it->get_sockfd() == sockfd){
			user_now = it;
		}
	}

	result << "*** " << user_now->get_nickname() << " yelled ***: " << message;
	for(it = user_table.begin(); it != user_table.end(); it++){
		write(it->get_sockfd(), result.str().c_str(), result.str().length());
		//c_str is used for converting c++ string type to c const char* type
	}
}