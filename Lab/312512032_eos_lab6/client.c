#include <fcntl.h>     // open()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // htons()
#include <errno.h>
#include <stdio.h>      // fprintf(), perror()
#include <stdlib.h>     // exit()
#include <string.h>     // memset()
#include <stdint.h>

#include <sys/socket.h> // socket
#include <signal.h>// signal() 
#include <unistd.h>    // read(), write(), close() ，dup2()
#include <sys/sem.h>

#define MYKEY 11223344
#define BUFFERSIZE 128

int sem;

int P(int s){
    struct sembuf sop;
    
    sop.sem_num=0; //binary
    sop.sem_op=-1; 
    sop.sem_flg=0;
    if (semop(s,&sop,1)<0){
        fprintf(stderr,"P(): semop failed: %s\n",strerror(errno));
        return -1;
    } else {
        return 0;   

    }
}
int V(int s){
    struct sembuf sop;
    
    sop.sem_num=0; //binary
    sop.sem_op=1; 
    sop.sem_flg=0;
    if (semop(s,&sop,1)<0){
        fprintf(stderr,"V(): semop failed: %s\n",strerror(errno));
        return -1;
    } else {
        return 0;   

    }
}


int main(int argc, char *argv[]) {
    if(argc != 6) {
        fprintf(stderr, "Usage: ./client <server_ip> <port> <deposit/withdraw> <amount> <times>");
        exit(EXIT_FAILURE);
    }
    int amount=atoi(argv[4]);
    int times=atoi(argv[5]);
    char send_buf[BUFFERSIZE]={0};
    sem=semget(MYKEY,1,0);
    
    if (sem < 0) {
    fprintf (stderr," cannot find semaphore %d: %s\n", MYKEY, strerror(errno));
    exit(1);
    }

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if ( socket_fd < 0) {
        printf("Create socket fail!\n");
        exit(EXIT_FAILURE);
    }
    //printf("socket has been created \n");
 
    struct sockaddr_in cli_Addr; 
    bzero(&cli_Addr,sizeof(cli_Addr));
    cli_Addr.sin_family = PF_INET;
    cli_Addr.sin_addr.s_addr = inet_addr(argv[1]);
    cli_Addr.sin_port = htons(atoi(argv[2]));
    if (connect(socket_fd, (struct sockaddr *)&cli_Addr, sizeof(cli_Addr)) == -1) {
        printf("Connect server failed!\n");
        close(socket_fd);
        exit(0);
    }
    //printf("Connect server [%s:%d] success\n",inet_ntoa(cli_Addr.sin_addr), ntohs(cli_Addr.sin_port));
   


    for(int count=0;count<times;count++){
        P(sem);
        sprintf(send_buf,"%s %d",argv[3],amount);
        //printf("%d\n",count);
        send(socket_fd, send_buf, BUFFERSIZE, 0);
        memset(send_buf,0,BUFFERSIZE);
        V(sem);
        usleep(1);
      
    }
    sprintf(send_buf,"exit");
    send(socket_fd, send_buf, BUFFERSIZE, 0);
    memset(send_buf,0,BUFFERSIZE);
    
    close(socket_fd);
    
    return 0;
}


