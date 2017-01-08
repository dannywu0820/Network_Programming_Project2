#include<sys/types.h>

#ifndef SERVICE_H
#define SERVICE_H

//Used in Project1
void serve(int);
void send_welcome_msg(int, char*);

//Used in Project2
void handle_new_connection(int , fd_set*, int*);
void handle_client_request(int, fd_set*);

#endif
