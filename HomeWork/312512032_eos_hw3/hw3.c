#include <fcntl.h>     // open()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // htons()
#include <errno.h>
#include <stdio.h>      // fprintf(), perror()
#include <stdlib.h>     // exit()
#include <string.h>     // memset()
#include <stdint.h>
#include <sys/ioctl.h> 
#include <sys/socket.h> // socket
#include <signal.h>// signal() 
#include <unistd.h>    // read(), write(), close() ，dup2()
#include <pthread.h>
#include <sys/sem.h>
#define BUFFERSIZE 256
#define MYKEY1 12345
#define MYKEY2 67890
int  server_socket;
int delivery_waiting_time[2]={0};
int sem1,sem2;

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




void* counter(){
    while (1){

        sleep(1);




        P(sem1);
        if (delivery_waiting_time[0]>0){
            
            delivery_waiting_time[0]=delivery_waiting_time[0]-1;
            
        }
        V(sem1);
        P(sem2);
        if (delivery_waiting_time[1]>0){
        
            delivery_waiting_time[1]=delivery_waiting_time[1]-1;
            
        }
        V(sem2);
    }
}


void* Delivery_platform(void* socket){   
        
        char receive[BUFFERSIZE]={0};
        char order_check[20]={0};char item_check[20]={0};
        int  order_num[2]={0};char send_data[BUFFERSIZE]={0},order_temp[BUFFERSIZE]={0}; 
        int sock=*(int*)socket;
        
        int flag=0;
        int price=0;
        order_num[0]=0;
        order_num[1]=0;
        int who =2;
        
        while((recv(sock,receive,BUFFERSIZE,0))){
            
            memset(send_data,0, BUFFERSIZE);
            printf("\nthreadid:%lu\n",pthread_self());
            printf("fd:%d\n",sock);
            printf("input:%s",receive);
            if(strcmp(receive,"cancel\n")==0 ||strcmp(receive,"No\n")==0){
                break;
            }
            else if(strcmp(receive,"Yes\n")==0){
                        int temp=0;
                        if(who==0){
                            P(sem1);
                            delivery_waiting_time[0]=delivery_waiting_time[0]+flag;
                            temp=delivery_waiting_time[0];
                            V(sem1);
                        }else if(who==1){
                            P(sem2);
                            delivery_waiting_time[1]=delivery_waiting_time[1]+flag;
                            temp=delivery_waiting_time[1];
                            V(sem2);
                        }
                        sprintf(send_data,"Please wait a few minutes...\n");
                        send(sock, send_data, BUFFERSIZE, 0);
                        printf("out:%s\n",send_data);
                        memset(send_data,0, BUFFERSIZE);
                        printf("\nfd:%d\nsec:%d\n",sock,temp);
                        for(int i=temp;i>0;i--){
                            sleep(1);
                        }

                        



                        sprintf(send_data,"Delivery has arrived and you need to pay %d$\n",price);
                        send(sock, send_data, BUFFERSIZE, 0);
                        printf("\nfd:%d\nout:%s\n",sock,send_data);
                        break;
            }
            else if(strcmp(receive,"shop list\n")==0){
                sprintf(send_data,"Dessert shop:3km\n- cookie:$60|cake:$80\nBeverage shop:5km\n- tea:$40|boba:$70\nDiner:8km\n- fried-rice:$120|Egg-drop-soup:$50\n");
                send(sock, send_data, BUFFERSIZE, 0);
                printf("out:%s\n",send_data);
            }
            else if(strcmp(receive,"confirm\n")==0){
                if (price==0){        
                    sprintf(send_data,"Please order some meals\n");
                    send(sock, send_data, BUFFERSIZE, 0);
                    printf("out:%s\n",send_data);    
                }else{
                    int temp_count=0;
                    
                    P(sem1);
                    int temp1=delivery_waiting_time[0]+flag;
                    V(sem1);
                    P(sem2);
                    int temp2=delivery_waiting_time[1]+flag;
                    V(sem2);

                    if((temp1<30)|| (temp2<30)){
                        if(temp1<temp2){
                            who=0;
                            P(sem1);
                            delivery_waiting_time[0]=temp1;
                            temp_count=temp1;
                            V(sem1);
                        }else{
                            who=1;
                            P(sem2);
                            delivery_waiting_time[1]=temp2;
                            temp_count=temp2;
                            V(sem2);
                        }
                
                        sprintf(send_data,"Please wait a few minutes...\n");
                        send(sock, send_data, BUFFERSIZE, 0);
                        printf("fd:%d\nwho:%d\nsec:%d\nreal:%d\nflag:%d\n",sock,who,temp_count,delivery_waiting_time[who],flag);
                        printf("out:%s\n",send_data);
                        memset(send_data,0, BUFFERSIZE);
                        for(int i=temp_count;i>0;i--){
                            sleep(1);
                        }
                      
                        sprintf(send_data,"Delivery has arrived and you need to pay %d$\n",price);
                        send(sock, send_data, BUFFERSIZE, 0);
                        printf("\nfd:%d\nout:%s\n",sock,send_data);
                        break;
                    }else{
                        if(temp1<temp2){
                            who=0;
                            temp_count=temp1;
                        }else{
                            who=1;
                            temp_count=temp2;
                        }
                        printf("fd:%d\nwho:%d\nsec:%d\nflag:%d",sock,who,temp_count,flag);
                        memset(send_data,0, BUFFERSIZE);
                        sprintf(send_data,"Your delivery will take a long time, do you want to wait?\n");
                        send(sock, send_data, BUFFERSIZE, 0);
                        printf("out:%s\n",send_data);
                    }    
                }
                
            }
            else{
                int count=0;
                int num_check=0;
                memset(item_check,0, sizeof(item_check));
                memset(order_check,0, sizeof(order_check));
                sscanf(receive,"%s %s %d\n",order_check, item_check,&num_check);
                if(strcmp(order_check,"order")==0){

                   
                    if(flag==0){
                        if (strcmp(item_check,"cookie")==0 || strcmp(item_check,"cake")==0 ){
                            flag=3;
                        } 
                        else if(strcmp(item_check,"tea")==0 || strcmp(item_check,"boba")==0 ){
                            flag=5;
                        } 
                        else if(strcmp(item_check,"fried-rice")==0 || strcmp(item_check,"Egg-drop-soup")==0 ){
                            flag=8;
                        } 
                    }
                    if((strcmp(item_check,"cookie")==0 || strcmp(item_check,"cake")==0 )&&flag==3){
                        
                        //which item and num
                        if(strcmp(item_check,"cookie")==0){
                        order_num[0]+=num_check; 
                        }else {
                            order_num[1]+=num_check;
                        }
                        //output
                        if(order_num[0]!=0){
                            count+=sprintf(order_temp+count,"cookie %d",order_num[0]);   
                            if(order_num[1]!=0 )
                                count+=sprintf(order_temp+count,"|");
                        }
                        if(order_num[1]!=0){
                            count+=sprintf(order_temp+count,"cake %d",order_num[1]);
                        }
                        count+=sprintf(order_temp+count,"\n");
                        price=60*order_num[0]+80*order_num[1];
                        
                    }
                    else if((strcmp(item_check,"tea")==0 || strcmp(item_check,"boba")==0 )&&flag==5){
                        
                        //which item and num
                        if(strcmp(item_check,"tea")==0){
                        order_num[0]+=num_check; 
                        }else {
                            order_num[1]+=num_check;
                        }
                        //output
                        if(order_num[0]!=0){
                            count+=sprintf(order_temp+count,"tea %d",order_num[0]);
                            if(order_num[1]!=0 )
                                count+=sprintf(order_temp+count,"|");
                        }

                        if(order_num[1]!=0){
                            count+=sprintf(order_temp+count,"boba %d",order_num[1]);
                        }
                        count+=sprintf(order_temp+count,"\n");
                        price=40*order_num[0]+70*order_num[1];

                    }
                    else if((strcmp(item_check,"fried-rice")==0 || strcmp(item_check,"Egg-drop-soup")==0)&&flag==8){
                        
                        //which item and num
                        if(strcmp(item_check,"fried-rice")==0){
                        order_num[0]+=num_check; 
                        }else {
                            order_num[1]+=num_check;
                        }
                        //output
                        if(order_num[0]!=0){
                            count+=sprintf(order_temp+count,"fried-rice %d",order_num[0]);
                            if(order_num[1]!=0 )
                                count+=sprintf(order_temp+count,"|");
                        }
                        if(order_num[1]!=0){
                            count+=sprintf(order_temp+count,"Egg-drop-soup %d",order_num[1]);
                        }
                        count+=sprintf(order_temp+count,"\n");
                        price=120*order_num[0]+50*order_num[1];
                    }   
                    else{
                        send(sock, order_temp, BUFFERSIZE, 0);
                        printf("out:%s\n",order_temp);
                       
                        continue;
                    }   
                    strcpy(send_data,order_temp);
                    send(sock, send_data, BUFFERSIZE, 0);
                    printf("out:%stod:%d %d\n",send_data,order_num[0],order_num[1]);
                   
                }
            }  
        }
        
        close(sock); 
        free(socket); 
        pthread_exit(NULL);
}




void sigint_handler(int signo) {
    
    if(semctl(sem1,0,IPC_RMID,0)<0){
        fprintf (stderr, " unable to remove semaphore1 %d\n", MYKEY1);
        exit(1);  
    }
    printf("\nSemaphore1 %d has been remove\n", MYKEY1);
    if(semctl(sem2,0,IPC_RMID,0)<0){
        fprintf (stderr, " unable to remove semaphore2 %d\n", MYKEY2);
        exit(1);  
    }
    printf("\nSemaphore2 %d has been remove\n", MYKEY2);
    close(server_socket);
    exit(0);
}
int main(int argc, char *argv[]) {
    
    int  client_socket;
    signal(SIGINT, sigint_handler);
/////////////semaohore////////////////////////
    sem1=semget(MYKEY1,1,IPC_CREAT | IPC_EXCL | 0666);
    if(sem1<0){
        fprintf(stderr,"cannot find semaohore1 %d :%s\n",MYKEY1,strerror(errno));
        exit(1);
    }
    if ( semctl(sem1, 0, SETVAL, 1) < 0 )
    {
        fprintf(stderr, "Unable to initialize Sem1: %s\n", strerror(errno));
        exit(0);
    }

    sem2=semget(MYKEY2,1,IPC_CREAT | IPC_EXCL | 0666);
    if(sem2<0){
        fprintf(stderr,"cannot find semaohore2 %d :%s\n",MYKEY2,strerror(errno));
        exit(1);
    }
    if ( semctl(sem2, 0, SETVAL, 1) < 0 )
    {
        fprintf(stderr, "Unable to initialize Sem2: %s\n", strerror(errno));
        exit(0);
    }
///////////////semaohore////////////////////////


    
    if(argc != 2) {
        fprintf(stderr, "Usage: ./lab5  <port>");
        exit(EXIT_FAILURE);
    }
   
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
   
    int yes = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in serverAddr;
    memset(&serverAddr,0,sizeof(serverAddr));
    
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=INADDR_ANY;
    serverAddr.sin_port =htons((uint16_t)atoi(argv[1]));
    
    if(bind(server_socket,(const struct sockaddr *)&serverAddr,sizeof(serverAddr))<0){
        perror("bind_error");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 100) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    

    pthread_t thread_id[100];
    int pthread_coiunt=0;

    pthread_t thread_counter;
    pthread_create(&thread_counter,NULL,counter,NULL);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        client_socket =  accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }else{
            int* clientfd_ptr=(int*) malloc(sizeof(int));
            if (clientfd_ptr== NULL){
                perror("Failed allocate");
                exit(EXIT_FAILURE);
            }
            *clientfd_ptr=client_socket;
            pthread_create(&thread_id[pthread_coiunt],NULL,Delivery_platform,(void*)clientfd_ptr);
            pthread_coiunt++;
           
        }        
    }
    close(server_socket);
    return 0;
}
