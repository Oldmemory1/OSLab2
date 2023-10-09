# 一、 实验目的
学习生产者与消费者的运行基本原理，学习使用共享内存区和信号量机制，\
学习使用多进程以及进程间通信的方法，学会使用锁互斥访问对象。
# 二、 实验内容
• 一个大小为 3 的缓冲区，初始为空\
• 2 个生产者\
– 随机等待一段时间，往缓冲区添加数据，\
– 若缓冲区已满，等待消费者取走数据后再添加\
– 重复 6 次\
• 3 个消费者\
– 随机等待一段时间，从缓冲区读取数据\
– 若缓冲区为空，等待生产者添加数据后再读取\
– 重复 4 次\
说明：\
• 显示每次添加和读取数据的时间及缓冲区里的数据（指缓冲区里的具体内容）\
• 生产者和消费者用进程模拟（不能用线程）\
• Linux/Window 下都需要做
# 三、 实验环境
Windows10\
VMWare16\
Ubuntu-18.04
# 四、 程序设计与实现
## 通用的简要设计思路：
### (1) 主进程
    负责创建公共信号量和共享内存区，以及创建并等待 2 个生产者子进程和 3 个消费者子进程
### (2) 共享内存区
    需要包括大小为 3 的缓冲区数组 a、缓冲区数据头部指针 beg、缓冲区数据尾部指针 end
### (3) 公共信号量
    共需 3 个，分别是：
    同步信号量 Filled，指示已填充的缓冲区个数，初值为 0
    同步信号量 Emptyed，指示为空的缓冲区个数，初值为 3
    互斥信号量 ReadWriteLock，类似读写锁，控制子进程互斥地使用缓冲区，初值为 1
### (4) 生产者子进程
    需要对共享内存区进行读写操作并在缓冲区内填入数据，
    这还涉及到信号量的申请与释放，其具体每次生产执行过程可以抽象为
    P(Emptyed); //申请空闲缓冲区资源
    P(ReadWriteLock); //申请对缓冲区修改的权限
    array[end]=x //在空缓冲区填入数据
    end=(end+1)%3 //尾部指针递增
    V(Filled); //填充了一个缓冲区，释放填充缓冲区信号量
    V(ReadWriteLock); //释放修改权限
    (5) 消费者子进程需要对共享内存区进行读写操作并在缓冲区内读出数据，
    其每次消费具体执行过程可以抽象为
    P(Filled); //申请已填充缓冲区资源
    P(ReadWriteLock); //申请对缓冲区修改的权限
    y=array[begin] //从已填充缓冲区读取数据
    beg=(beg+1)%3 //头部指针递增
    V(Emptyed); //读取了一个缓冲区，释放空缓冲区信号量
    V(ReadWriteLock); //释放修改权限
### (6) 生产者消费者子进程全部运行结束后
    主进程需要回收共享内存区和信号量
## 两个操作系统更具体的设计实现思路
    为方便查看，先说明宏定义
    生产者总数#define producerAmount 2  
    一个生产者生产出的数据量#define productProducedByOneProducer 6
    消费者总数#define consumerAmount 3
    一个消费者消费的数据量#define productConsumedByOneConsumer 4
    进程总量#define processAmount 5
    缓冲区大小#define bufferSize 3
### Windows10
#### (1)主进程
     主进程需要负责创建各对象，首先需要创建共享内存区
     共享内存区创建完成后，需要创建信号量
     创建信号量完成后，需创建 5 个子进程，
     本次实验子进程和主进程都是在一个.c文件中实现，
     区别在于主进程只含一个参数，
     而子进程还包含进程的ID，指明子进程序号，主函数结构参考如下
     int main(int argc,char *argv[]){
     if(argc==1){
         //run main process
     }
     else{
         //run sub process
     int ID=atoi(argv[1]);
     if(ID is producer){
         Producer(ID)
     }else if(ID is consumer){
         Consumer(ID)
     }
     return 0;
     }
     在主进程创建子进程调用 CreateProcess 函数以传入命令行的形式创建，
     并且需要保存创建的子进程句柄到 Handle_process 数组内
     各对象创建完毕后主进程进入阻塞状态，
     调用 WaitForMultipleObjects等待Handle_process 数组
     内句柄对应的所有子进程结束。然后关闭子进程句柄、信号量和共享内存区句柄
#### (2)共享内存区
     struct sharedMemory{
     int a[bufferSize];
     int begin;
     int end;
     };
     共享内存区在主进程中创建，需要函数 CreateFileMapping 创建内存中临时文件映射对象，
     对象名称为“sharedMemory”，
     大小为自定义的共享内存区结构 sharedMemory，
     然后通过函数MapViewOfFile 将文件映射对象的一个视口映射到主进程，完成临时文件初始化操作
     共享内存区是生产者和消费者子进程共享使用，
     在生产者和消费者子进程开始执行操作前，
     必须先通过函数 OpenFileMapping 打开之前创建的临时文件对象并获取句柄，
     然后通过函数 MapViewOfFIle 将共享的临时文件对象的一个视口映射到当前进程的地址空间，
     并定义一个相同结构体的指针指向该地址，
     然后就可以读取共享内存区数据了，注意最后要关闭句柄，解除映射
#### (3)信号量句柄
     Emptyed Filled ReadWriteLock是全局变量
     在主进程中调用自定义的 Create_Mux 函数创建信号量，
     在其中调用了CreateSemaphore 函数来创建各个信号量
     之后在所有子进程的使用中，
     首先需要使用 OpenSemapHore 打开对应信号量
     在子进程信号量使用相关内容结束后，同样需要关闭信号量句柄
#### (4)生产者子进程
     获取参数 ID 后，需要先在进程打开共享内存区的临时文件映射对象并将一个视口映射到当前进程地址空间，
     最终会获得指向共享内存区结构的指针 sm，然后调用自定义函数 Open_Mux 在当前进程内获取各个信号量句柄，
     然后即可开始执行操作。每个生产者均需要重复 productProducedByOneProducer 次生产操作，
     故需要一个循环结构，在每次循环内，生产者首先调用 Sleep 等待随机的一段时间，然后开始正式的生产执行操作，
     首先使用函数 WaitForSingleObject模拟 P 操作
     然后就是往缓冲区生产填充数据，这里需要通过指针 sm 进行操作
     然后用 ReleaseSemaphore 函数释放信号量
     重复 Rep_Producer 次生产过程，关闭信号量句柄，解除文件映射，关闭临时文件句柄，该生产者子进程结束
#### (5)消费者子进程
     每个消费者均需要重复 productConsumedByOneConsumer 次消费操作，
     故需要一个循环结构，在每次循环内，消费者首先调用 Sleep 等待随机的一段时间，
     然后开始正式的消费执行操作，首先使用函数 WaitForSingleObject 模拟 P 操作
     然后用 ReleaseSemaphore 函数释放信号量
     重复 Rep_Consumer 次消费过程，关闭信号量句柄，解除文件映射，关闭临时文件句柄，该消费者子进程结束
#### (6)所有子进程结束后
     WaitForMultipleObject 函数结束，关闭子进程句柄、信号量和共享内存区句柄，主进程结束
#### (7)运行结果
     见文件夹下的1.png 2.png 3.png
     ![picture1][https://github.com/Oldmemory1/OSLab2/blob/master/1.png]
     ![picture2][https://github.com/Oldmemory1/OSLab2/blob/master/2.png]
     ![picture3][https://github.com/Oldmemory1/OSLab2/blob/master/3.png]
### Ubuntu18.04




