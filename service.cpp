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
#include<fcntl.h>

#include<vector>
using namespace std;

#define BUF_SIZE 30000
#define NUM_OF_CMD 30000
#define NUM_OF_RCD 1000
struct cmd{
    int type; //0:normal 1:|n 2:> xxx.txt 3:!n
    int argn;
    char *args[1800];
};

struct record{
    int readOnly_fd;
    int writeOnly_fd;
    int count_down;
};
vector<struct record> table;

void send_welcome_msg(int sockfd, char *buffer);
int parse_received_msg(char *r_buffer, struct cmd* cmds);
int execute_cmds(int num_of_cmd, struct cmd* cmds, int sockfd);
void execute_single_cmd(struct cmd command, int i, int input_fd, int output_fd, int stdout_fd, bool input_from_past, int err_input_fd);

int read_previous_pipe();
////////////////////////////////////////////////////////////////////////////
void handle_client_requests(int slave_sockfd, fd_set* fdset_ref){
	char r_buffer[BUF_SIZE];
    int r_bytes;
	
	bzero(r_buffer, BUF_SIZE);
    r_bytes = read(slave_sockfd, r_buffer, BUF_SIZE);
	
	if(r_bytes == 0){ //when a client quits
	    close(slave_sockfd);
	    FD_CLR(slave_sockfd, fdset_ref);
		printf("close socket file descriptor %d\n", slave_sockfd);
		printf("------------------------------------------\n");
	}
}

void serve(int sockfd){
    char w_buffer[BUF_SIZE], r_buffer[BUF_SIZE];
    int w_bytes, r_bytes;

    //Step1.send welcome messages
    send_welcome_msg(sockfd, w_buffer);

    sprintf(w_buffer, "%% ");
    w_bytes = write(sockfd, w_buffer, strlen(w_buffer));
    while(1){

        //Step2.read messages from client
        bzero(r_buffer, BUF_SIZE);
        r_bytes = read(sockfd, r_buffer, BUF_SIZE);
        for(int j = 0; j < strlen(r_buffer); j++){
           if(r_buffer[j] == 13) r_buffer[j] = '\0';
        }

        if(strstr(r_buffer, "exit") != NULL){
            break;
        }

        //Step3.parse received messages
        struct cmd* cmds = (struct cmd*)malloc(sizeof(struct cmd)*NUM_OF_CMD);
        int num_of_cmd;
        num_of_cmd = parse_received_msg(r_buffer, cmds);

        /*printf("/////%d/////\n",num_of_cmd);
        for(int j = 0; j < num_of_cmd; j++){
            printf("cmd[%d] ", j+1);
            printf("type%d: ", cmds[j].type);
            for(int k = 0; k < cmds[j].argn; k++){
                printf("%s ", cmds[j].args[k]);
            }
            printf("\n");
        }*/
       
        int result_fd = execute_cmds(num_of_cmd, cmds, sockfd);
        bzero(w_buffer, BUF_SIZE);
        read(result_fd, w_buffer, BUF_SIZE);
        //w_buffer[strlen(w_buffer)] = '\n';
        write(sockfd, w_buffer, strlen(w_buffer));
 
        free(cmds);

        //Step4.
        bzero(w_buffer, BUF_SIZE);
        sprintf(w_buffer, "%% ");
        w_bytes = write(sockfd, w_buffer, strlen(w_buffer));
        if(w_bytes < 0) printf("failed write()\n");

    }

    table.clear();
}

void send_welcome_msg(int sockfd, char *buffer){
    sprintf(buffer, "****************************************\n");
    write(sockfd, buffer, strlen(buffer));
    sprintf(buffer, "** Welcome to the information server. **\n");
    write(sockfd, buffer, strlen(buffer));
    sprintf(buffer, "****************************************\n");
    write(sockfd, buffer, strlen(buffer));
}

int parse_received_msg(char *r_buffer, struct cmd* cmds){
    char *token[30000];
    char *token_ptr;
    int num_of_token = 0;

    token_ptr = strtok(r_buffer, " ");
    while(token_ptr != NULL){
        token[num_of_token] = token_ptr;
        num_of_token++;
        token_ptr = strtok(NULL, " ");
    }
        
    int cmdPos[30000]={0};
    //for(int j = 0; j < 30000; j++) cmdPos[j] = 0;
    int num_of_cmd = 1;
        
    for(int j = 0; j < num_of_token; j++){
        //printf("token[%d]:%s\n", j,token[j]);
        if(token[j][0] == '|' || token[j][0] == '>' || token[j][0] == '!'){
            cmdPos[num_of_cmd] = j;
            num_of_cmd++;
        }
    }
    cmdPos[num_of_cmd] = num_of_token;
        
    for(int j = 0; j < num_of_cmd; j++){
        //printf("cmd[%d]: ", j+1);
        bool skip_pipe = false;
        cmds[j].argn = 0;
        if(token[cmdPos[j]][0] == '|' && strlen(token[cmdPos[j]]) == 1) skip_pipe = true;
        if(token[cmdPos[j]][0] == '|' && !skip_pipe){ cmds[j].type = 1; }
        else if(token[cmdPos[j]][0] == '!'){ cmds[j].type = 2; }
        else if(token[cmdPos[j]][0] == '>'){ cmds[j].type = 3; }
        else{ cmds[j].type = 0; }
        for(int k = cmdPos[j]; k < cmdPos[j+1]; k++){
            //printf("%s ", token[k]);
            if(skip_pipe){
                skip_pipe = false;
                continue;
            }
            cmds[j].args[cmds[j].argn] = token[k];
            cmds[j].argn++;    
        }
        //printf("\n");
        cmds[j].args[cmds[j].argn] = NULL;
    }
    
    return num_of_cmd;
}

int execute_cmds(int num_of_cmd, struct cmd* cmds, int sockfd){
    int out_pipe[2];
    int err_pipe[2];
    int result_pipe[2];

    if(pipe(out_pipe) == -1) printf("failed pipe()\n");
    if(pipe(err_pipe) == -1) printf("failed pipe()\n");
    if(pipe(result_pipe) == -1) printf("failed pipe()\n");

    //save original fd of stdin, stdout, stderr
    int stdin_fd = dup(0);
    int stdout_fd = dup(1);
    int stderr_fd = dup(2);

    int input_fd = read_previous_pipe(); //read args of current cmd from here
    bool input_from_previous_pipe = (input_fd == 0)?false:true;
    int output_fd = out_pipe[1]; //write the output of a command to this
    int next_input_fd = out_pipe[0]; //read next cmd's input from here
    int err_input_fd = err_pipe[0];
    int err_output_fd = err_pipe[1];

    for(int i = 0; i < num_of_cmd; i++){
        //change the value of file descriptor table 0,1
        //to the designated fd as new stdin and stdout
        dup2(input_fd, 0);
        dup2(output_fd, 1);
        dup2(err_output_fd, 2);

        execute_single_cmd(cmds[i], i,input_fd, output_fd, stdout_fd, input_from_previous_pipe, result_pipe[0]);
        input_from_previous_pipe = false; //used when i==0

        //stdin,stdout
        if(i != 0) close(input_fd); //can't close stdin
        close(output_fd);
    
        if(pipe(out_pipe) == -1) printf("failed pipe()\n");
        input_fd = next_input_fd;
        output_fd = out_pipe[1];
        next_input_fd = out_pipe[0];

        //stderr
        char err_buffer[BUF_SIZE];
        bzero(err_buffer, BUF_SIZE);
        //turn read into non-blocking
        int original_flags = fcntl(err_input_fd, F_GETFL, 0);
        fcntl(err_input_fd, F_SETFL, original_flags|O_NONBLOCK);
        read(err_input_fd, err_buffer, BUF_SIZE);
        write(result_pipe[1], err_buffer, strlen(err_buffer));

        close(err_input_fd);
        close(err_output_fd);

        if(pipe(err_pipe) == -1) printf("failed pipe()\n");
        err_input_fd = err_pipe[0];
        err_output_fd = err_pipe[1];
    }

    //change the value back to original stdin and stdout
    dup2(stdin_fd, 0);
    dup2(stdout_fd, 1);
    dup2(stderr_fd, 2);

    vector<struct record>::iterator it;
    printf("talbe size:%d\n",table.size());
    for(it = table.begin(); it != table.end(); it++){
        printf("readOnly:%d writeOnly:%d count_down:%d\n", it->readOnly_fd, it->writeOnly_fd, it->count_down);
    }
    printf("/////////////\n");

    char input_buffer[BUF_SIZE];
    bzero(input_buffer, BUF_SIZE);
    int flags = fcntl(input_fd, F_GETFL, 0);
    fcntl(input_fd, F_SETFL, flags|O_NONBLOCK);
    read(input_fd, input_buffer, BUF_SIZE);
    write(result_pipe[1], input_buffer, strlen(input_buffer));
    close(result_pipe[1]);

    //return input_fd;    
    return result_pipe[0];    
}
void execute_single_cmd(struct cmd command, int i, int input_fd, int output_fd, int stdout_fd, bool input_from_past, int err_input_fd){
    char buffer[BUF_SIZE];
    bzero(buffer,BUF_SIZE);

    if(command.type == 0){
        if(strcmp(command.args[0], "printenv") == 0){
            char* result = getenv(command.args[1]);
            sprintf(buffer, "PATH:%s\n", result);
            write(output_fd, buffer, strlen(buffer));
        }
        else if(strcmp(command.args[0], "setenv") == 0){
            setenv(command.args[1], command.args[2], 1);
            write(output_fd, buffer, strlen(buffer));
        }
        else{
            vector<struct record>::iterator it;
            for(it = table.begin(); it != table.end(); it++){
                if(it->count_down == 0){
                    break;
                }
            }

            pid_t pid= fork();
            int status;
            if(pid < 0){ exit(1); }
            else if(pid == 0){ //child
                if(input_from_past) close(it->writeOnly_fd);
                execvp(command.args[0], command.args);
                char error_msg[100];
                sprintf(error_msg, "Unknown command:[%s].\n", command.args[0]);
                write(output_fd, error_msg, strlen(error_msg));
                exit(1);
            }
            else{ //parent
                if(input_from_past) close(it->writeOnly_fd);
                waitpid(pid, &status, 0);
                if(input_from_past){
                    close(it->readOnly_fd);
                    table.erase(it);
                }
            }
        }
    }
    else if(command.type == 1){ //|n
        int count;
        sscanf(command.args[0], "|%d", &count);
        
        bool create_new_pipe = true;
        vector<struct record>::iterator it;
        for(it = table.begin(); it != table.end(); it++){
            if(it->count_down == count){
                create_new_pipe = false;
                break;
            }
        }
        
        int newpipe_fd[2];
        if(create_new_pipe){
            //create a new pipe;
            pipe(newpipe_fd);
            //record the new pipe information
            struct record temp;
            temp.count_down = count;
            temp.readOnly_fd = newpipe_fd[0];
            temp.writeOnly_fd = newpipe_fd[1];
            table.push_back(temp);
        }
        
        //read from input_fd and write to output_fd
        read(input_fd, buffer, BUF_SIZE);
        //write(output_fd, buffer, strlen(buffer));
        if(create_new_pipe){
            write(newpipe_fd[1], buffer, strlen(buffer));
        }
        else{
            write(it->writeOnly_fd, buffer, strlen(buffer));
        }
    }
    else if(command.type == 2){ //!n
        int count;
        sscanf(command.args[0], "!%d", &count);
        
        bool create_new_pipe = true;
        vector<struct record>::iterator it;
        for(it = table.begin(); it != table.end(); it++){
            if(it->count_down == count){
                create_new_pipe = false;
                break;
            }
        }

        int newpipe_fd[2];
        if(create_new_pipe){
            //create a new pipe;
            pipe(newpipe_fd);
            //record the new pipe information
            struct record temp;
            temp.count_down = count;
            temp.readOnly_fd = newpipe_fd[0];
            temp.writeOnly_fd = newpipe_fd[1];
            table.push_back(temp);
        }
        
        //read from input_fd and write to output_fd
        char err_buffer[BUF_SIZE];
        bzero(err_buffer, BUF_SIZE);
        read(err_input_fd, err_buffer, BUF_SIZE);
        read(input_fd, buffer, BUF_SIZE);
        //write(output_fd, buffer, strlen(buffer));
        if(create_new_pipe){
            write(newpipe_fd[1], err_buffer, strlen(err_buffer));
            write(newpipe_fd[1], buffer, strlen(buffer));
        }
        else{
            write(it->writeOnly_fd, err_buffer, strlen(err_buffer));
            write(it->writeOnly_fd, buffer, strlen(buffer));
        }
    }
    else{ //>
        FILE* fptr = fopen(command.args[1], "w");
        read(input_fd, buffer, BUF_SIZE);
        write(fileno(fptr), buffer, strlen(buffer));
    }
}

int read_previous_pipe(){ //read input from previous cmd using pipe saved in table
    vector<struct record>::iterator it;
    for(it = table.begin(); it != table.end(); it++){
            it->count_down--;
            if(it->count_down == 0){
                return it->readOnly_fd;
            }
        }
    return 0;
}
