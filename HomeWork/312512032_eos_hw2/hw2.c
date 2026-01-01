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

#define BUFFERSIZE 256


int  server_socket;
int  client_socket;
struct server_address;
struct sockaddr_in client_addr;
socklen_t client_addr_len = sizeof(client_addr);

char order_check[20];
char item_check[20];
char num_check[10];
int  order_num[2];
char send_data[BUFFERSIZE],receive[BUFFERSIZE],order_temp[BUFFERSIZE];

void sigint_handler(int signo) {
    close(server_socket);
    exit(0);
}
int main(int argc, char *argv[]) {
    signal(SIGINT, sigint_handler);
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

    if (listen(server_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
        
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        
        //printf("\n\n\n");
        int flag=0;
        int price=0;
        order_num[0]=0;
        order_num[1]=0;
        
        while((recv(client_socket,receive,BUFFERSIZE,0))){
            int i=0;
            memset(send_data,0, BUFFERSIZE);
            //printf("%s\n",receive);

            if(strcmp(receive,"cancel")==0){
                break;
            }
            else if(strcmp(receive,"shop list")==0){
                sprintf(send_data,"Dessert shop:3km\n- cookie:$60|cake:$80\nBeverage shop:5km\n- tea:$40|boba:$70\nDiner:8km\n- fried-rice:$120|Egg-drop-soup:$50\n");
                send(client_socket, send_data, BUFFERSIZE, 0);
            }

            else if(strcmp(receive,"confirm")==0){
                //printf("%d\n",price);
                if (price==0){        
                    sprintf(send_data,"Please order some meals\n");
                    send(client_socket, send_data, BUFFERSIZE, 0);    

                }else{
                    sprintf(send_data,"Please wait a few minutes...\n");
                    send(client_socket, send_data, BUFFERSIZE, 0);
                    memset(send_data,0, BUFFERSIZE);
                    sleep(flag);
                    sprintf(send_data,"Delivery has arrived and you need to pay %d$\n",price);
                    send(client_socket, send_data, BUFFERSIZE, 0);
                    break;
                    
                }
                
            }else{
                while (receive[i]!=' '){
                    order_check[i]=receive[i];
                    i++;
                }
                if(strcmp(order_check,"order")==0){
                    int j=0;
                    int k=0;
                    int count=0;
                    memset(item_check,0, sizeof(item_check));
                    memset(num_check,0, sizeof(num_check));

                    while(receive[j+i+1]!=' '){
                        item_check[j]=receive[j+6];
                        j++;
                    }
                    while(receive[k+i+j+2]!=0){
                        num_check[k]=receive[k+i+j+2];
                        k++;
                    }
                    //printf("%d %d\n",j,k);
                    //printf("item:%s\n",item_check);
                    ///printf("num:%d\n",atoi(num_check));
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
                        order_num[0]+=atoi(num_check); 
                        }else {
                            order_num[1]+=atoi(num_check);
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
                        order_num[0]+=atoi(num_check); 
                        }else {
                            order_num[1]+=atoi(num_check);
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
                        order_num[0]+=atoi(num_check); 
                        }else {
                            order_num[1]+=atoi(num_check);
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
                        send(client_socket, order_temp, BUFFERSIZE, 0);
                        continue;
                    }   
                    strcpy(send_data,order_temp);
                    send(client_socket, send_data, BUFFERSIZE, 0);
                         
                }
            }  
        }  
        close(client_socket);       
    }
    close(server_socket);
    return 0;

}
