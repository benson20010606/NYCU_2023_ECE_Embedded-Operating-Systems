/*
* server.c: Parking lot server
*/

// Lib
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h> // sockaddr related
#include <sys/socket.h>
#include <sys/wait.h>


#define NUM_SPACES 16
#define SEM_MODE 0666 /* rw(owner)-rw(group)-rw(other) permission */
#define SEM_KEY_MUTEX 1122334454
#define SEM_KEY_COUNTING 45510
#define SHM_KEY_CAR 26
#define SHM_KEY_CASH 4423
#define SHM_KEY_SPACE 8787 // Ingram edit
#define SHM_KEY_SPACE_LEFT 7891 //莊edit

// Gloabal Variables
int socket_fd;     // Socket file descriptor
int sem_mutex, sem_counting;     // Semaphore
int shmid_car, shmid_cash, shmid_space, shmid_left;      // shared memory, Ingram edit, 莊edit
int asset = 0;     // total asset
int* park_space; // Identify whether a parking space is occupied, Ingram edit
int* space_left;    // Identify how much parking space left, space_left[0] for fuel, space_left[1] for electric, 莊edit

struct CAR{
    time_t entry_t;    // Entry time
    time_t pay_t;         // Payment time
    int elec;          // Electric car or not
    char licence_plate[8];    // License plate
    int pos;
};

struct CAR *car;
int *cash;

/* P () - returns 0 if OK; -1 if there was a problem */
int P (int s){
    struct sembuf sop; /* the operation parameters */
    sop.sem_num = 0; /* access the 1th sem in the array */
    sop.sem_op = -1; /* wait..*/
    sop.sem_flg = 0; /* no special options needed */
    if (semop (s, &sop, 1) < 0){
        fprintf(stderr,"P(): semop failed: %s\n",strerror(errno));
        return -1;
    }
    else{
        return 0;
    }
}

/* V() - returns 0 if OK; -1 if there was a problem */
int V(int s){
    struct sembuf sop; /* the operation parameters */
    sop.sem_num = 0; /* the 1st sem in the array */
    sop.sem_op = 1; /* signal */
    sop.sem_flg = 0; /* no special options needed */
    if (semop(s, &sop, 1) < 0){
        fprintf(stderr,"V(): semop failed: %s\n",strerror(errno));
        return -1;
    } 
    else{
        return 0;
    }
}

int entry(int fd, int entry_num, char *plate) {  // Ingram edit
    int pos = -1;
    char buf[256] = {};
    int drv_fd; //driver file descriptor
    char drv_buf[2] = {0};
    
    int x;
    int found = -1;
    for(x= 0; x < NUM_SPACES ; x++){
        if(strcmp(car[x].licence_plate, plate) == 0){
            found=x;
            break;
        }
    }
    if(found != -1){
        printf("already parked\n\n\n\n=====================\n\n");
        sprintf(buf, "already parked\n");
        send(fd, buf, 256,0);
        close(fd);
        return 0;}
    
    
    /* acquire semaphore */
    P(sem_mutex);
    /**************** Critical Section *****************/
        if(plate[0] == 'E'){
            if (entry_num == 1) {
                for (int i = 0; i < 8; i++) {
                    if (park_space[i] == 0) {
                        park_space[i] = 1;
                        pos = i;
                        break;
                    }
                }
            }
            else { // entry_num=2
                for (int i = 0; i < 8; i++) {
                    if (park_space[7-i] == 0) {
                        park_space[7-i] = 1;
                        pos = 7 - i;
                        break;
                    }
                }
            }

            if (pos == -1) { // no space is found
                /* release semaphore */
                V(sem_mutex);

                printf("No space(E)...\n\n\n\n=====================\n\n");
                sprintf(buf, "No space\n");
                send(fd, buf, 256,0);
                close(fd);
              
                return 0;
            }
            space_left[1]--; 
            car[pos].elec = 1;
        }
        else{
            if (entry_num == 1) {
                for (int i = 0; i < 8; i++) {
                    if (park_space[i+8] == 0) {
                        park_space[i+8] = 1;
                        pos = i + 8;
                        break;
                    }
                }
            }
            else {
                for (int i = 0; i < 8; i++) {
                    if (park_space[15-i] == 0) {
                        park_space[15-i] = 1;
                        pos = 15 - i;
                        break;
                    }
                }
            }

            if (pos == -1) { // no space is found
                /* release semaphore */
                V(sem_mutex);

                printf("No space(F)...\n\n");
                sprintf(buf, "No space\n");
                send(fd, buf, 256,0);
             
                close(fd);
                return 0;
            }
            space_left[0]--; 
            car[pos].elec = 0;
        }
        strcpy(car[pos].licence_plate, plate);
        car[pos].entry_t = time(NULL);
        car[pos].pay_t = 0;
        car[pos].pos = pos;
    /**************** Critical Section *****************/
    /* release semaphore */
    V(sem_mutex);
    
    drv_fd = open("/dev/etx_device", O_RDWR); // file descriptor
    drv_buf[0] = (char)(pos);
    drv_buf[1] = (char)0;
    write(drv_fd, drv_buf, sizeof(drv_buf));
    close(drv_fd);
        
    
    
    sprintf(buf, "Entry Success\n");
    send(fd, buf, 256,0);


    close(fd);
    printf("PID[%d]: Car[%s] is parked at slot[%d], elec=%d, entry_time=%ld, num_elec=%d, num_fuel=%d\n\n\n\n=====================\n\n",
     getpid(), car[pos].licence_plate, car[pos].pos, car[pos].elec, car[pos].entry_t, space_left[1], space_left[0]);

    return 0;
}

int exitpark(int fd, char *plate){
    int pos = -1;
    char buf[256] = {};
    int drv_fd; //driver file descriptor
    char drv_buf[2] = {0};
    
    
    int x;
    int find =-1;
    for(x= 0; x < NUM_SPACES; x++){
        if(strcmp(car[x].licence_plate, plate) == 0){
            find=x;
            break;
        }
    }if(find ==-1){
        printf("Car doesn't exist!!\n\n\n\n=====================\n\n");
        sprintf(buf, "Car doesn't exist\n");
        send(fd, buf, 256,0);
        close(fd);
        return 0;
    }
    
    
    
    
    for(int i = 0; i < NUM_SPACES; i++){
        if(strcmp(car[i].licence_plate, plate) == 0){
            pos = i;
            break;
        }
    }

    if(car[pos].pay_t == 0){
        
        sprintf(buf, "Exit Failed, Please pay the bill\n");
        send(fd, buf, 256,0);
        printf("Exit Failed, Please pay the bill\n\n\n=====================\n\n");
        return -1;
    }
    time_t t = time(NULL);
    time_t tpass = t - car[pos].pay_t;
    if(tpass > 25){
        /* acquire semaphore */
        P(sem_mutex);
        /**************** Critical Section *****************/
            car[pos].entry_t = car[pos].pay_t;
            car[pos].pay_t = -1;
        /**************** Critical Section *****************/
        /* release semaphore */
        V(sem_mutex);
        
        sprintf(buf, "Exit Failed, Please Repay the bill\n");
        send(fd, buf, 256,0);
      
        printf("Exit Failed, Please Repay the bill\n\n\n=====================\n\n");
        return -1;
    }
    else{
        /* acquire semaphore */
        P(sem_mutex);
        /**************** Critical Section *****************/
            strcpy(car[pos].licence_plate, "");
            car[pos].entry_t = -1;
            car[pos].pay_t = -1;
            space_left[car[pos].elec]++;    //莊edit
            car[pos].elec = -1;
            park_space[pos] = 0;
        /**************** Critical Section *****************/
        /* release semaphore */
        V(sem_mutex);
        sprintf(buf, "Exit Success\n");
        send(fd, buf, 256,0);
        printf("Total Income: %d$\n\n", *cash);
        printf("Exit Success\n\n\n=====================\n\n");
        
        drv_fd = open("/dev/etx_device", O_RDWR); // file descriptor
        drv_buf[0] = (char)(pos);
        drv_buf[1] = '1'-48;
        write(drv_fd, drv_buf, sizeof(drv_buf));
        close(drv_fd);
        
        return 0;
    }
}

void payment(int fd, char *plate){
    char buf[256] = {};
    int fee = 0;
    
    /* acquire counting semaphore */
    P(sem_counting);
    /**************** Critical Section *****************/

        int i;
        for(i = 0; i < NUM_SPACES; i++){
            if(strcmp(car[i].licence_plate, plate) == 0){
                break;
            }
        }
        
        
        if(i < NUM_SPACES){
        time_t t = time(NULL);
        time_t tpass = t - car[i].entry_t;
        car[i].pay_t = t;
      
            if(car[i].elec){
                fee = 30*(tpass/30 + 1);    //30$ per 30 min for electric car
            }
            else{
                fee = 20*(tpass/30 + 1);    //20$ per 30 min for non-electric car
            }
            sprintf(buf, "Fee:%d\n",fee);
            printf("Plate: %s\nPos: %d\nPark time: %ld minutes\nTotal Fee: %d$\n", car[i].licence_plate, car[i].pos, tpass, fee);
            write(fd, buf, 256);

            read(fd, buf, 256);
            while(atoi(buf) != fee){
                write(fd, "Wrong Amount\n", 256);
                printf("Wrong Amount\n");
                sprintf(buf, "Fee:%d$\n",fee);
                write(fd, buf, 256);

                read(fd, buf, 256);
            }
            
            /* acquire semaphore */
            P(sem_mutex);
            /**************** Critical Section *****************/
                car[i].pay_t = t;
            /**************** Critical Section *****************/
            /* release semaphore */
            V(sem_mutex);

            /* acquire semaphore 17*/
            P(sem_mutex);
            /**************** Critical Section *****************/
               *cash += fee;
            /**************** Critical Section *****************/
            /* release semaphore 17*/
            V(sem_mutex);
            sprintf(buf, "Plate: %s\nPos: %d\nPark time: %ld minutes\nTotal Fee: %d$\nPayment Success\nPlease exit within 25 minutes\n", car[i].licence_plate, car[i].pos, tpass, fee);
            printf("Payment Success\nPlease exit within 25 minutes\n\n=====================\n\n");
            write(fd, buf, 256);
             close(fd);
        }
        else{
            sprintf(buf, "Cannot find your car\n");
            printf("Cannot find your car\n\n\n=====================\n\n");
            write(fd, buf, 256);
           
            close(fd);
        }
    /**************** Critical Section *****************/
    /* release counting semaphore */
    V(sem_counting);

    exit(0);
}

void handler(int signum){
        
    if (sem_mutex < 0)
    {
        fprintf(stderr,    "failed to find semaphore: %s\n", strerror(errno));
    }
    
    /* remove semaphore */
    if (semctl(sem_mutex, 0, IPC_RMID, 0) < 0)
    {
        fprintf (stderr, "unable to remove sem %d\n", SEM_KEY_MUTEX);
        
    }else
    {
        printf("\nSemaphore %d has been remove\n", SEM_KEY_MUTEX);
    }

    if (sem_counting < 0)
    {
        fprintf(stderr,    "failed to find semaphore: %s\n", strerror(errno));
    }
    
    /* remove semaphore */
    if (semctl(sem_counting, 0, IPC_RMID, 0) < 0)
    {
        fprintf (stderr, "unable to remove sem %d\n", SEM_KEY_COUNTING);
        
    }else
    {
        printf("\nSemaphore %d has been remove\n", SEM_KEY_COUNTING);
    }

    shmdt(car);
    /* Destroy the share memory segment */
    printf("Server destroy the share memory.\n");
    int retval = shmctl(shmid_car, IPC_RMID, NULL);
    if (retval < 0)
    {
        fprintf(stderr, "Server remove share memory failed\n");
        exit(1);
    }

    shmdt(cash);
    /* Destroy the share memory segment */
    printf("Server destroy the share memory.\n");
    retval = shmctl(shmid_cash, IPC_RMID, NULL);
    if (retval < 0)
    {
        fprintf(stderr, "Server remove share memory failed\n");
        exit(1);
    }

    shmdt(park_space); // Ingram edit
    /* Detach the shared memory segment */
    printf("Server destroy the share memory.\n");
    retval = shmctl(shmid_space, IPC_RMID, NULL);
    if (retval < 0)
    {
        fprintf(stderr, "Server remove share memory failed\n");
        exit(1);
    }

    shmdt(space_left); //莊edit
    /* Detach the shared memory segment */
    printf("Server destroy the share memory.\n");
    retval = shmctl(shmid_left, IPC_RMID, NULL);
    if (retval < 0)
    {
        fprintf(stderr, "Server remove share memory failed\n");
        exit(1);
    }

    /* close socket */    
    close(socket_fd);
    printf("Socket has been close\n");
    exit(0);
}

void handler_fork(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]){
        
    int port = atoi(argv[1]);    // port
    int connfd;            // client's socket file descriptor
    int yes = 1;
    int childpid;
    
    int drv_fd; //driver file descriptor
    char drv_buf[2] = {0};

    
    
    struct sockaddr_in info = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(port)
    };
    
    struct sockaddr_in cliaddr;
    
    int cliaddr_len = sizeof(cliaddr);
    
    // Close the socket and Semaphores
    signal(SIGINT, handler);
    signal(SIGCHLD, handler_fork);
    
    // Create a socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd < 0) printf("Fail to create a socket...");
    
    // Force using socket address already in use
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    
    // bind()
    if(bind(socket_fd, (const struct sockaddr *)&info, sizeof(info)) < 0){
        perror("Bind socket failed!");
        close(socket_fd);
        exit(0);
    }
    
    // listen()
    listen(socket_fd, 10);
    

    /* create 16 mutex semaphore for parking lot 1~16, 17 for cash */
    sem_mutex = semget(SEM_KEY_MUTEX, 1, IPC_CREAT | IPC_EXCL | SEM_MODE);
    if (sem_mutex < 0)
    {
        fprintf(stderr, "Sem %d creation failed: %s\n", SEM_KEY_MUTEX, strerror(errno));
        exit(-1);
    }
    /* initial semaphore value to 1 (binary semaphore) */
    if ( semctl(sem_mutex, 0, SETVAL, 1) < 0 )
    {
        fprintf(stderr, "Unable to initialize Mutex Sem %d: %s\n", 1, strerror(errno));
        exit(0);
    }

    /* create counting semaphore */
    sem_counting = semget(SEM_KEY_COUNTING, 1, IPC_CREAT | IPC_EXCL | SEM_MODE);
    if (sem_counting < 0)
    {
        fprintf(stderr, "Sem %d creation failed: %s\n", SEM_KEY_COUNTING, strerror(errno));
        exit(-1);
    }
    /* initial semaphore value to 2 (2 pay stations) (counting semaphore) */
    if ( semctl(sem_counting, 0, SETVAL, 2) < 0 )
    {
        fprintf(stderr, "Unable to initialize Counting Sem: %s\n", strerror(errno));
        exit(0);
    }
    
    /* Create the segment size 16*sizeof(CAR) */
    if ((shmid_car = shmget(SHM_KEY_CAR, sizeof(struct CAR)*16, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    /* Now we attach the segment to our data space */
    if ((car = shmat(shmid_car, NULL, 0)) == (struct CAR *) -1) {
        perror("shmat");
        exit(1);
    }

    /* Create the segment for cash */
    if ((shmid_cash = shmget(SHM_KEY_CASH, sizeof(int), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    /* Now we attach the segment to our data space */
    if ((cash = shmat(shmid_cash, NULL, 0)) == (int *) -1) {
        perror("shmat");
        exit(1);
    }

    /* Create the segment for parking space, Ingram edit */
    if ((shmid_space = shmget(SHM_KEY_SPACE, NUM_SPACES* sizeof(int), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    /* Now we attach the segment to our data space */
    if ((park_space = shmat(shmid_space, NULL, 0)) == (int *) -1) {
        perror("shmat");
        exit(1);
    }

    /* Create the segment for space left, 莊edit */
    if ((shmid_left = shmget(SHM_KEY_SPACE_LEFT, 2 * sizeof(int), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    /* Now we attach the segment to our data space */
    if ((space_left = shmat(shmid_left, NULL, 0)) == (int *) -1) {
        perror("shmat");
        exit(1);
    }

    space_left[0] = 8;  //init space is 8, 莊edit
    space_left[1] = 8;  //init space is 8, 莊edit
    *cash = 0;

    char ipBuf[256] = {};    // Message received
    // char Buf[256] = {};
        for(int i=0;i<NUM_SPACES;i++) {
        printf("hihi\n");
        drv_fd = open("/dev/etx_device", O_RDWR); // file descriptor
        drv_buf[0] = (char)i;
        drv_buf[1] ='1'-48;
        write(drv_fd, drv_buf, sizeof(drv_buf));
        close(drv_fd);
    }
    
    while(1){
        int entry_num = 0; // Ingram edit
        char method[8]; // Ingram edit
        char plate[8]; // Ingram edit

        connfd = accept(socket_fd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        
        memset(ipBuf,'\0',256);
        recv(connfd, ipBuf, sizeof(ipBuf), 0);
        printf("\nReceived: %s\n",ipBuf);
        
        if ((childpid = fork()) > 0){ /* parent */
            
            close(connfd);
        }

        else if (childpid == 0){ /* child */
            sscanf(ipBuf, "%[^!]!%d!%s", method, &entry_num, plate); // Spit the msg from client, Ingram edit

            if(strcmp(method, "Entry") == 0) // Entry, Ingram edit
            {
                /*************************/
                /****   ENTRY         ****/
                /*************************/

                entry(connfd, entry_num, plate);   //input connfd, pos of parking lot, licence plate, Ingram edit
            }
            else if(strcmp(method, "Exit") == 0) // Exit, Ingram edit
            {
                /*************************/
                /****    EXIT         ****/
                /*************************/

                exitpark(connfd, plate);      //input connfd, pos of parking lot, licence plate
            }
            else if(strcmp(method, "Pay") == 0) // Payment, Ingram edit
            {
                /*************************/
                /****    PAYMENT      ****/
                /*************************/

                payment(connfd, plate);      //input connfd
            }
            exit(0);
        }
    }


    return 0;
}
