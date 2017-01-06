#include"service.h"

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

#include<iostream>
#include<vector>
#include<ctime>

#define QUEUE_LEN 32 //maximum connection queue length
#define BUF_SIZE 30000
////////////////////////////////////////////////////////////////////////
void Initial_Setup(){
	setenv("PATH", "bin:.", 1);
	
	char initial_dir[50] = "";
	strcat(initial_dir, getenv("HOME"));
	strcat(initial_dir, "/rwg");
    chdir(initial_dir);
	//printf("%s\n", initial_dir);
}

void Reaper(int sig){
    int status;
    while(waitpid(-1, &status, WNOHANG) >= 0);
	//-1 means wait until any one child process terminates
	//WNOHANG flag specifies that waitpid should return immediately instead of waiting
	//if there is no child process ready to be noticed
}

void Prevent_Zombie(){
	//When a child process exits before parent
	//the exit code of child will be kept in process table, which makes it a zombie
	//waiting for parent to collect(use wait() or waitpid()) or until parent exits
	signal(SIGCHLD, Reaper);
	//When a child process exits(asynchronous event), the kernel will send SIGCHLD signal to parent
	//and function Reaper will be called
}

int errno; //error number used for strerror()
unsigned short portbase = 0; //what's this?

int error_exit(const char *format, ...){
    va_list args; //list of arguments
    
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(-1);
}

int passive_sock(const char *service, const char *transport, int q_len){
    struct sockaddr_in addr; //an Internet endpoint address
    int sock_fd, sock_type;
    struct servent  *pse;   /* pointer to service information entry */
    struct protoent *ppe;   /* pointer to protocol information entry*/
    
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if(pse = getservbyname(service, transport))
        addr.sin_port = htons(ntohs((unsigned short)pse->s_port) + portbase);
    else if((addr.sin_port=htons((unsigned short)atoi(service))) == 0)
        error_exit("can't get \"%s\" service entry\n", service);
    
    if((ppe = getprotobyname(transport)) == 0)
        error_exit("can't get \"%s\" protocol entry\n", transport);

    if(strcmp(transport, "udp") == 0) sock_type = SOCK_DGRAM;
    else sock_type = SOCK_STREAM;

    //1.create a master socket
    sock_fd = socket(AF_INET, sock_type, 0); //create socket
    if(sock_fd < 0) error_exit("failed socket(): %s\n", strerror(sock_fd));

    //2.bind to an address(IP:port)
    errno = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
    if(errno < 0) error_exit("failed bind(): %s\n", service, strerror(errno));
    
    //3.place the master in passive mode for accepting connection requests
    if(sock_type == SOCK_STREAM && listen(sock_fd, q_len) < 0){
        error_exit("failed listen(): %s\n", service, strerror(sock_type));
    }
 
    return sock_fd; 
}

void handle_new_connection(int master_sockfd, fd_set* fdset_ref, int* max_fd){
	struct sockaddr_in client_addr;
	socklen_t client_addrlen;
	int new_sockfd = accept(master_sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
	
	if(new_sockfd != -1){
		char from[INET_ADDRSTRLEN];
        printf("new connection from ");
        printf("%s\n", inet_ntop(AF_INET, &(client_addr.sin_addr), from, INET_ADDRSTRLEN));
        printf("create socket file descriptor %d to handle\n", new_sockfd);
		printf("------------------------------------------\n");
		
        FD_SET(new_sockfd, fdset_ref);
        *max_fd = (new_sockfd > (*max_fd)?new_sockfd:(*max_fd));
		//printf("max_fd changed to %d\n", *max_fd);
		
		char w_buffer[BUF_SIZE];
        int w_bytes;
		send_welcome_msg(new_sockfd, w_buffer);
		
		sprintf(w_buffer, "%% ");
        w_bytes = write(new_sockfd, w_buffer, strlen(w_buffer));
	}
	else{
		error_exit("failed accept()\n");
	}
}

int main(int argc, char *argv[]){

    char *service; //int server_port;
    struct sockaddr_in client_addr;
    int master; //master sockfd(socked file descriptor)
    unsigned int addrlen; //length of client's address

	if(argc != 2) error_exit("usage: %s port\n", argv[0]);
    service = argv[1];

    Prevent_Zombie();
	Initial_Setup();
	
	master = passive_sock(service, "tcp", QUEUE_LEN);
	
	int max_fd = master, num_of_fd;
	fd_set listened_fd_set, read_fd_set; //listened_set contains fds being listened, read_set contains fds want to read
	FD_ZERO(&listened_fd_set);
    FD_SET(master, &listened_fd_set);
	
    while(1){
		struct timeval timeout;
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		
		num_of_fd = max_fd+1;
        read_fd_set = listened_fd_set;
		int status = select(num_of_fd, &read_fd_set, NULL, NULL, &timeout);
		//return the number of file descriptors contained in the three returned descriptor sets
        if(status == 0){
			//printf("timeout, no activities detected\n");
		}
		else if(status > 0){
			for(int fd = 0; fd <= num_of_fd; fd++){
				if(FD_ISSET(fd, &read_fd_set)){
					if(fd == master){ //master handles new connections
						handle_new_connection(fd, &listened_fd_set, &max_fd);
					}
					else{ //slaves handle commands from clients
						handle_client_requests(fd, &listened_fd_set);
					}
				}
			}
		}
		else{
			error_exit("failed select(): %s\n", strerror(status));
		}
		
    }
	
	close(master);
}
