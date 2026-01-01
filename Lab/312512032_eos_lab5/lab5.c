#include <signal.h>// signal() 
#include <sys/wait.h> // waitpid
#include <fcntl.h>     // open()
#include <unistd.h>    // read(), write(), close() ，dup2()
#include <stdio.h>      // fprintf(), perror()
#include <stdlib.h>     // exit()
#include <string.h>     // memset()
#include <sys/socket.h> // socket
#include <sys/types.h>
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // htons()
#include <stdint.h>
   
int  fd;
struct server_address;

int  client_socket;
struct sockaddr_in client_addr;
socklen_t client_addr_len = sizeof(client_addr);



void sigint_handler(int signo) {
    close(fd);
}

void handler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
    
    
}


int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Usage: ./lab5  <port>");
        exit(EXIT_FAILURE);
    }
   
    

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    signal(SIGINT, sigint_handler); // 註冊  signal 
     
    signal(SIGCHLD, handler); // 註冊  signal 
    
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr,0,sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=INADDR_ANY;
    serverAddr.sin_port =htons((uint16_t)atoi(argv[1]));
    
    if(bind(fd,(const struct sockaddr *)&serverAddr,sizeof(serverAddr))<0){
        perror("bind_error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (listen(fd, 5) < 0) {
        perror("listen");
        return 1;
    }
    while (1) {
        client_socket = accept(fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }
        
       
        pid_t childpid;
        childpid = fork();
        
        if (childpid < 0) {
            perror("fork");
            exit(1);
        }

        if (childpid == 0) { // 子進程
            dup2(client_socket, STDOUT_FILENO);
            execlp("/usr/games/sl","sl","-l",NULL);
 
            exit(1);
        } else { // 父進程
            printf("Train ID: %d\n", childpid);
            close(client_socket);
        }
    }

    close(fd);
    
}
