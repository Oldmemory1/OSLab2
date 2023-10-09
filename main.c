#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<windows.h>

#define producerAmount 2
#define productProducedByOneProducer 6
#define consumerAmount 3
#define productConsumedByOneConsumer 4
#define processAmount 5
#define bufferSize 3


struct sharedMemory
{
    int a[bufferSize];
    int begin;
    int end;
};

HANDLE Emptyed,Filled,ReadWriteLock;
HANDLE Handle_process[processAmount+5];

int getRandomNumber(int minimum_number,int max_number){
    return (rand() % (max_number + 1 - minimum_number)) + minimum_number;
}

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

void show_buffer(struct sharedMemory *sm)
{
    printf("buffer: ");
    for(int i=0;i<bufferSize;i++)
    {
        printf("%d ",sm->a[i]);
    }
    printf("\n");
}

HANDLE MakeSharedFile()
{
    HANDLE hMapping=CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0, sizeof(struct sharedMemory),"SHARE_MEM");
    LPVOID pData=MapViewOfFile(hMapping,FILE_MAP_ALL_ACCESS,0,0,0);
    ZeroMemory(pData, sizeof(struct sharedMemory));
    UnmapViewOfFile(pData);
    return(hMapping);
}


void Create_Mux()
{
    Emptyed=CreateSemaphore(NULL, bufferSize, bufferSize, "SEM_EMPTY");
    Filled=CreateSemaphore(NULL, 0, bufferSize, "SEM_FILL");
    ReadWriteLock=CreateSemaphore(NULL, 1, 1, "SEM_RW");
}


void Open_Mux()
{
    Emptyed=OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE,"SEM_EMPTY");
    Filled=OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE,"SEM_FILL");
    ReadWriteLock=OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE,"SEM_RW");
}

void Close_Mux()
{
    CloseHandle(Emptyed);
    CloseHandle(Filled);
    CloseHandle(ReadWriteLock);
}

void New_SubProcess(int ID)
{
    STARTUPINFO si;
    memset(&si,0,sizeof(si));
    si.cb=sizeof(si);
    PROCESS_INFORMATION pi;

    char Cmdstr[105];
    char CurFile[105];

    GetModuleFileName(NULL, CurFile, sizeof(CurFile));
    sprintf(Cmdstr, "%s %d", CurFile, ID);

    CreateProcess(NULL,Cmdstr,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
    Handle_process[ID]=pi.hProcess;

    return;
}

void Producer(int ID)
{
    HANDLE hMapping=OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,"SHARE_MEM");
    LPVOID pFile = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    struct sharedMemory *sm=(struct sharedMemory*)(pFile);
    Open_Mux();
    for(int i=0;i<productProducedByOneProducer;i++)
    {
        srand((unsigned int)time(NULL) + ID);



        int temp=(rand()%5+1)*1000;
        Sleep(temp);

        int randomNumber= getRandomNumber(-127,128);

        WaitForSingleObject(Emptyed,INFINITE);
        WaitForSingleObject(ReadWriteLock,INFINITE);

        printf("%d号进程:生产者在%d号缓冲区生产%d:",ID,sm->end,randomNumber);
        format_time();
        sm->a[sm->end]=randomNumber;
        sm->end=(sm->end+1)%bufferSize;
        show_buffer(sm);

        ReleaseSemaphore(Filled,1,NULL);
        ReleaseSemaphore(ReadWriteLock,1,NULL);
    }
    Close_Mux();
    UnmapViewOfFile(pFile);
    CloseHandle(hMapping);
}

void Consumer(int ID)
{
    HANDLE hMapping=OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,"SHARE_MEM");
    LPVOID pFile = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    struct sharedMemory *sm=(struct sharedMemory*)(pFile);
    Open_Mux();
    for(int i=0;i<productConsumedByOneConsumer;i++)
    {
        srand((unsigned int)time(NULL) + ID);
        int temp=(rand()%5+1)*1000;
        Sleep(temp);

        WaitForSingleObject(Filled,INFINITE);
        WaitForSingleObject(ReadWriteLock,INFINITE);

        printf("%d号进程:消费者在%d号缓冲区消费%d:",ID,sm->begin,sm->a[sm->begin]);
        format_time();
        sm->a[sm->begin]=0;
        sm->begin=(sm->begin+1)%bufferSize;
        show_buffer(sm);

        ReleaseSemaphore(Emptyed,1,NULL);
        ReleaseSemaphore(ReadWriteLock,1,NULL);
    }
    Close_Mux();
    UnmapViewOfFile(pFile);
    CloseHandle(hMapping);
}


int main(int argc,char *argv[])
{
    if(argc==1)
    {
        HANDLE hMapping=MakeSharedFile();
        Create_Mux();
        printf("0号进程:创建子进程，1~2号为生产者进程，3~5号为消费者进程\n");
        for(int i=1;i<=processAmount;i++)
        {
            New_SubProcess(i);
        }
        WaitForMultipleObjects(processAmount,Handle_process+1,TRUE,INFINITE);
        for(int i=1;i<=processAmount;i++)
        {
            CloseHandle(Handle_process[i]);
        }
        Close_Mux();
        printf("0号进程:子进程运行完成\n");
        CloseHandle(hMapping);
    }
    else
    {
        int ID=atoi(argv[1]);
        if(ID<=producerAmount)
            Producer(ID);
        else
            Consumer(ID);
    }

    return 0;
}