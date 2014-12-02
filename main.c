#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>




//|||||||||||||||||||调试框||||||||||||||||||||||||||||||||

     #define copy_times 15           //拷贝次数
//   #define clear_all              //清除过去的信号灯和缓冲区
     #define sleep_time 1           //定义暂停时间
//   #define test                   //是否启用调试功能
     #define FILEPATH "./file.txt"  //读取文件的目录
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||


//定义这个数据结构
union semun { 
int val;	/* value for SETVAL */ 
struct semid_ds *buf;	/* buffer for IPC_STAT, IPC_SET */ 
unsigned short int *array; 	/* array for GETALL, SETALL */ 
struct seminfo *__buf;	/* buffer for IPC_INFO */ 
}; 


//共享内存ID
static int shmid1;
static int shmid2;

void P(int semid,int index);
void V(int semid,int index);

//三个子程序
void get();
void copy();
void put();


int main(){
    
    int semid;
    
    //创建四个信号灯
    if((semid = semget((key_t)1222, 4, IPC_CREAT|0666))==-1)
    {
        printf("创建信号灯失败:%d\n",errno);
        exit(1);
    }
    
    //初始四个信号灯
    union semun sem;
    sem.val=0;
    if( semctl(semid, 0, SETVAL,sem) == -1)
    {
        printf("信号灯1初始化错误:%d\n",errno);
        exit(1);
    }
    if( semctl(semid, 1, SETVAL,sem) == -1)
    {
        printf("信号灯2初始化错误:%d\n",errno);
        exit(1);
    }
    sem.val=1;
    if( semctl(semid, 2, SETVAL,sem) == -1)
    {
        printf("信号灯3初始化错误:%d\n",errno);
        exit(1);
    }
    if( semctl(semid, 3, SETVAL,sem) == -1)
    {
        printf("信号灯4初始化错误:%d\n",errno);
        exit(1);
    }
    
    
    //创建共享内存
    if((shmid1 = shmget((key_t)1222, 4096,IPC_CREAT|0666)) == -1)
    {
        printf("创建共享内存失败:%d\n",errno);
        exit(1);
    }
    
    if((shmid2 = shmget((key_t)1223, 4096,IPC_CREAT|0666)) == -1)
    {
        printf("创建共享内存失败:%d\n",errno);
        exit(1);
    }
    
   
//上一次调试失败后留下的信号灯和共享内存没有清除
#if defined clear_all
    
    //销毁共享内存
    shmctl(shmid1,IPC_RMID,0);
    shmctl(shmid2,IPC_RMID,0);
    //销毁信号灯
    semctl(semid, 0, IPC_RMID);
    semctl(semid, 1, IPC_RMID);
    semctl(semid, 2, IPC_RMID);
    semctl(semid, 3, IPC_RMID);
    
    //直接退出
    return 0;
#endif
    
    
    
    //创建三个子程序
    if(fork() == 0)
    {
        copy();
        return 0;
    }else if(fork() == 0)
    {
        get();
        return 0;
    }else if(fork() == 0)
    {
        put();
        return 0;
    }
    
    
    
    //等待三个子程序执行完毕
    int id;
    wait(&id);
    wait(&id);
    wait(&id);
    
    //销毁共享内存
    if(shmctl(shmid1,IPC_RMID,0) == -1)
    {
        printf("销毁共享内存错误：%d\n",errno);
        exit(1);
    }
    if(shmctl(shmid2,IPC_RMID,0) == -1)
    {
        printf("销毁共享内存错误：%d\n",errno);
        exit(1);
    }
    
    //销毁信号灯
    semctl(semid, 0, IPC_RMID);
    semctl(semid, 1, IPC_RMID);
    semctl(semid, 2, IPC_RMID);
    semctl(semid, 3, IPC_RMID);
    
}

//P操作
void P(int semid,int index)
{
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_flg = 0;
    sem.sem_op = -1;
    
    if( semop(semid, &sem, 1) == -1)
    {
        printf("执行P操作失败\n");
    }
    
}

//V操作
void V(int semid,int index)
{
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_flg = 0;
    sem.sem_op = 1;
    
    if( semop(semid, &sem, 1) == -1 )
    {
        printf("执行V操作失败\n");
    }
    
}




void get()
{
    void* shm;  //缓冲区1的绑定地址
    int semid;  //信号灯1和信号灯3
    
    //绑定缓冲区1
    if( (int)(shm = shmat(shmid1, NULL, SHM_R|SHM_W)) == -1)
    {
        printf("get函数绑定缓冲区1错误：%d\n",errno);
        exit(1);
    }
    
    //获得信号灯1和3
    if( (int)(semid = semget((key_t)1222, 1, IPC_PRIVATE)) == -1)
    {
        printf("get获得信号灯错误：%d\n",errno);
        exit(1);
    }
    
    //设置读取文件
    FILE* fp;
    char buffer[1024];
    fp = fopen(FILEPATH,"r");
    
    
   // printf("执行get开始\n");
    //向缓冲区1中写数据
    for(int i=0;i<copy_times;i++)
    {
        P(semid,2);
        
     //   read(fd, buffer, sizeof(buffer));
        
        fgets(buffer,1000,fp);
        
       // printf("%s", buffer);
        
        strcpy(shm,buffer);
        
       // FILE* fp;
        
#if defined test
        
        printf("get一次\n");
        sleep(sleep_time);
        
#endif
    
        V(semid,0);
    }
    
    //关闭读取
    fclose(fp);
}



void copy()
{
    void* shm1;
    void* shm2;  //缓冲区1和2的绑定地址
    int semid;  //信号灯
    
    //绑定缓冲区1
    if( (int)(shm1 = shmat(shmid1, NULL, SHM_R|SHM_W)) == -1)
    {
        printf("copy函数绑定缓冲区1错误：%d\n",errno);
        exit(1);
    }
    
    if( (int)(shm2 = shmat(shmid2, NULL, SHM_R|SHM_W)) == -1)
    {
        printf("copy函数绑定缓冲区2错误：%d\n",errno);
        exit(1);
    }
    
    //获得信号灯
    if( (int)(semid = semget((key_t)1222, 1, IPC_PRIVATE)) == -1)
    {
        printf("copy获得信号灯错误：%d\n",errno);
        exit(1);
    }
    
   // printf("执行copy开始\n");
    //向缓冲区1中写数据
    for(int i=0;i<copy_times;i++)
    {
        P(semid,3);
        P(semid,0);
        
        //从缓冲区1拷贝到缓冲区2
        strcpy((char*)shm2,(char*)shm1);
        
        
#if defined test
        
        printf("copy一次\n");
        sleep(sleep_time);
        
#endif
        
        V(semid,1);
        V(semid,2);
    }
}



void put()
{
    void* shm;  //缓冲区2的绑定地址
    int semid;  //信号灯
    
    //绑定缓冲区1
    if( (int)(shm = shmat(shmid2, NULL, SHM_R|SHM_W)) == -1)
    {
        printf("put函数绑定缓冲区2错误：%d\n",errno);
        exit(1);
    }
    
    //获得信号灯1和3
    if( (int)(semid = semget((key_t)1222, 1, IPC_PRIVATE)) == -1)
    {
        printf("put获得信号灯错误：%d\n",errno);
        exit(1);
    }
    
 //   printf("执行put开始\n");
    //向缓冲区1中写数据
    for(int i=0;i<copy_times;i++)
    {
        P(semid,1);
        
        char* message=malloc(4096);
        
        //从缓冲区2中拷贝出来
        strcpy(message,(char*)shm);
        
        //向屏幕打印出来
        printf("%s",message);
        free(message);
        
#if defined test
        
        printf("put一次\n");
        sleep(sleep_time);
        
#endif
        
        V(semid,3);
    }
}




