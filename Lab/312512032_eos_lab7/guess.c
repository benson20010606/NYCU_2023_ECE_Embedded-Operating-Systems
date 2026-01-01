/*
todo:取得 shared memory 的定址空間。
todo:本程式使用 對半猜(二分搜) 的方式來做猜測的動作。
todo:Game 程式使用 SIGUSR1 信號來通知 Game 程式去接收判斷結果。
todo:撰寫 timer，每隔一秒讀取 result 變數的結果，計算出該回合要猜的數字並寫入 guess 變數當中，最後使用 SIGUSR1 通知 Game 程式處理。
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
typedef struct {
    int guess;
    char result[8];
}data_t;

data_t *my_data,*shm_data;
int shmid;
int upper_bound,guess;
int lower_bound=1;
key_t key;
pid_t pid;


void guess_handler (int signum)
{
    if (strcmp(my_data->result, "smaller") == 0){
        upper_bound=guess-1;
    }else if (strcmp(my_data->result, "bigger") == 0){
        lower_bound=guess+1;

    }else if (strcmp(my_data->result, "bingo") == 0){
        shmdt(shm_data); 
        exit(0);
    }
    guess=(upper_bound+lower_bound)/2;
    my_data->guess=guess;
    printf("[game] Guess  (%d~%d) :%d \n",lower_bound,upper_bound,guess);
    kill(pid, SIGUSR1);
}



int main(int argc, char *argv[])
{
    struct sigaction sa;
    struct itimerval timer;
    if(argc != 4) {
        fprintf(stderr, "./guess <key> <upper_bound> <pid>");
        exit(EXIT_FAILURE);
    }

    key=atoi(argv[1]);
    upper_bound=atoi(argv[2]);
    pid=atoi(argv[3]);

    /* Create the segment */
    if ((shmid = shmget(key,sizeof(my_data),  0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    /* Now we attach the segment to our data space */
    if ((shm_data = (data*)shmat(shmid, NULL, 0)) == (data*) -1) {
        perror("shmat");
        exit(1);
    }
    printf("guess.c attach the share memory created by game.c \n");
    /* Now put some things into the memory for the other process to read */
    my_data=shm_data;


    /* Install timer_handler as the signal handler for SIGVTALRM */
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &guess_handler;
    sigaction (SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    /* Reset the timer back to 1 sec after expired */
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    /* Start a virtual timer */
    setitimer (ITIMER_REAL, &timer, NULL);



    while(1);
    
    return 0;

}
