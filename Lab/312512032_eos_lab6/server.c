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
#include <pthread.h>

#define BUFFERSIZE 128
#define MYKEY 11223344
#define SEM_MODE 0666
int  server_socket;
int  client_socket;
int s;
int my_money_now=0;
//pthread_mutex_t mutex;
//int a=1;
//int b=1;
void sigint_handler(int signo) {
    close(server_socket);
    //pthread_mutex_destroy(&mutex);
    if(semctl(s,0,IPC_RMID,0)<0){
        fprintf (stderr, " unable to remove semaphore %d\n", MYKEY);
        exit(1);  
    }
    printf("\nSemaphore %d has been remove\n", MYKEY);
    printf("socket  has been close\n");
    exit(0);
}


void* bank(void* socket){   
        char receive[BUFFERSIZE]={0};
        char what_to_do[10]={0}; 
        int sock=*(int*)socket;
        while((recv(sock,receive,BUFFERSIZE,0))){
            //pthread_mutex_lock(&mutex);
            int money=0;
            //printf("%s\n",receive);
            sscanf(receive,"%s %d",what_to_do,&money);
            //printf("%d\n",money);
            if(strcmp(what_to_do,"deposit")==0){
                my_money_now=my_money_now+money;
                printf("After deposit(%d):%d\n",money,my_money_now);//a++;
            }
            else if(strcmp(what_to_do,"withdraw")==0){
                my_money_now=my_money_now-money;
                printf("After withdraw(%d):%d\n",money, my_money_now);//b++;
            }else if(strcmp(what_to_do,"exit")==0){
            	break;
            }
            
            //pthread_mutex_unlock(&mutex);
        }
        pthread_exit(NULL);
}



int main(int argc, char *argv[]) {

    //pthread_mutex_init(&mutex,NULL);
    signal(SIGINT, sigint_handler);
   

    if(argc != 2) {
        fprintf(stderr, "Usage: ./server  <port>");
        exit(EXIT_FAILURE);
    }


    s=semget(MYKEY,1,IPC_CREAT | IPC_EXCL | SEM_MODE);
    if(s<0){
        fprintf(stderr,"cannot find semaohore %d :%s\n",MYKEY,strerror(errno));
        exit(1);
    }
    if ( semctl(s, 0, SETVAL, 1) < 0 )
    {
        fprintf(stderr, "Unable to initialize Sem: %s\n", strerror(errno));
        exit(0);
    }
    //printf("Semaphore %d has been created & initialized to 1\n", MYKEY);


    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket()");
        semctl(s,0,IPC_RMID,0);
        exit(EXIT_FAILURE);
    }
    //printf("server_socket has been created \n");

    int yes = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    
    struct sockaddr_in serverAddr={
    .sin_family=PF_INET,
    //.sin_addr.s_addr=inet_addr("127.0.0.1"),
    .sin_addr.s_addr=INADDR_ANY,
    .sin_port =htons(atoi(argv[1]))
    };
 
    if(bind(server_socket,(const struct sockaddr *)&serverAddr,sizeof(serverAddr))<0){
        perror("bind_error");
        semctl(s,0,IPC_RMID,0);
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    //printf("bind finished \n");
    if (listen(server_socket, 10) < 0) {
        perror("listen");
        semctl(s,0,IPC_RMID,0);
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    //printf("listen finished \n");
   
    pthread_t thread_id[100];
    int i=0;
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        pthread_create(&thread_id[i],NULL,bank,(void*)&client_socket);
        i++;
    }
return 0;
}
