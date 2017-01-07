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

#ifndef SERVICE_H
#define SERVICE_H

//Used in Project1
void serve(int);
void send_welcome_msg(int, char*);

//Used in Project2
void handle_new_connection(int , fd_set*, int*);
void handle_client_request(int, fd_set*);

#endif
