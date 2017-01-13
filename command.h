#include"user.h"

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

#include<vector>
using namespace std;

#ifndef COMMAND_H
#define COMMAND_H

#define BUF_SIZE 30000
class Command{
	public:
		void who(vector<User> user_table, int sockfd);
		void name(vector<User>* user_table, int sockfd, char* name);
		void tell(vector<User> user_table, int sockfd, char* message, int recv_sockfd);
		void yell(vector<User> user_table, int sockfd, char* message);
};

#endif