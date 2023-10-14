//
// Created by user on 23-10-14.
//
#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<unistd.h>
#include <time.h>

#define Cnt_Producer 2
#define Rep_Producer 6
#define Cnt_Consumer 3
#define Rep_Consumer 4
#define Cnt_Process 5
#define Len_buffer 3

#define SHMKEY 1024
#define SEMKEY 2048
void format_time(){
    time_t rawTime;
    struct tm * timeInfo;

    time(&rawTime);
    timeInfo = localtime(&rawTime);
    printf("current time:");

    printf("[%d-%d-%d %d:%d:%d]", timeInfo->tm_year + 1900,timeInfo->tm_mon + 1,timeInfo->tm_mday,
           timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    printf("\n");
}


struct share_memory
{
    int a[Len_buffer];
    int beg;
    int end;
};

void show_buffer(struct share_memory *sm)
{
    printf("buffer data: ");
    for(int i=0;i<Len_buffer;i++)
    {
        printf("%d ",sm->a[i]);

    }
    printf("-");
    format_time();
    printf("\n");
}

int Create_Mux()
{
    int semid=semget(SEMKEY,3,0666|IPC_CREAT);
    semctl(semid,0,SETVAL,3); //empty
    semctl(semid,1,SETVAL,0); //fill
    semctl(semid,2,SETVAL,1); //rw
    return semid;
}


void P(int semid,int n)
{
    struct sembuf temp;
    temp.sem_num=n;
    temp.sem_op=-1;
    temp.sem_flg=0;
    semop(semid,&temp,1);
}

void V(int semid,int n)
{
    struct sembuf temp;
    temp.sem_num=n;
    temp.sem_op=1;
    temp.sem_flg=0;
    semop(semid,&temp,1);
}

int Init_Share_Memory()
{
    int shmid=shmget(SHMKEY,sizeof(struct share_memory),0666|IPC_CREAT);
    struct share_memory* sm=(struct share_memory*)shmat(shmid,0,0);
    sm->beg=sm->end=0;
    for(int i=0;i<Len_buffer;i++)
    {
        sm->a[i]=0;
    }
    return shmid;
}

void Producer(int ID,int shmid,int semid)
{
    struct share_memory *sm=(struct share_memory*)shmat(shmid,0,0);
    for(int i=0;i<Rep_Producer;i++)
    {
        srand((unsigned)time(NULL));
        int temp=(rand()%3+1);
        sleep(temp);
        P(semid,0);
        P(semid,2);

        printf("Process %d:Producer produce %d at buffer %d at ",ID,temp,sm->end);
        format_time();
        sm->a[sm->end]=temp;
        sm->end=(sm->end+1)%Len_buffer;
        show_buffer(sm);

        V(semid,1);
        V(semid,2);

    }
    shmdt(sm);
}

void Consumer(int ID,int shmid,int semid)
{
    struct share_memory *sm=(struct share_memory*)shmat(shmid,0,0);
    for(int i=0;i<Rep_Consumer;i++)
    {
        srand((unsigned)time(NULL));
        int temp=(rand()%3+1);
        sleep(temp);
        P(semid,1);
        P(semid,2);

        printf("Process %d:Consumer consume %d at buffer %d at ",ID,sm->a[sm->beg],sm->beg);
        format_time();
        sm->a[sm->beg]=0;
        sm->beg=(sm->beg+1)%Len_buffer;

        show_buffer(sm);
        V(semid,0);
        V(semid,2);
    }
    shmdt(sm);
}

int main(int argc,char *argv[])
{
    int semid=Create_Mux();
    int shmid=Init_Share_Memory();

    printf("Process 0:Create Sub Process,1-2 is producer process,3-5 is consumer process\n");

    for(int i=1;i<=Cnt_Process;i++)
    {
        int pid=fork();
        if(pid==0)
        {
            //子进程
            if(i<=Cnt_Producer)
                Producer(i,shmid,semid);
            else
                Consumer(i,shmid,semid);
            return 0;
        }
    }
    //主进程
    for(int i=1;i<=Cnt_Process;i++)
    {
        wait(NULL);
    }
    semctl(semid,IPC_RMID,0);
    shmctl(shmid,IPC_RMID,0);
    printf("process 0: all sub processes have finished\n");
    return 0;
}