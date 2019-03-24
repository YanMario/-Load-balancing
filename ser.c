//
// Created by yanpan on 2019/3/23.
//

#if 1
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>

typedef struct sock
{
    int c;
    int sockfd;
}sock;

typedef struct worker
{
    void *(*Process) (void *arg);
    void* arg;
    struct worker *next;
}Pthread_Worker;

typedef struct
{
    bool m_stop;
    pthread_t *threadid_arr;  //描述线程池的数组
    int m_max_requests;  //请求队列允许最大的请求数
    int max_num;         //线程池中线程的最大数目
    int deque_size;      //工作队列的大小
    int shutdown;        //销毁线程池
    int list1_num;      //请求队列的数目
    pthread_mutex_t mutex;  //保护请求队列的锁
    pthread_cond_t cond;    //条件变量
    Pthread_Worker *list1;    //请求队列
}Pthread_pool;

static Pthread_pool* pool = NULL;
void run()
{

}
void* Clib_Accept(void *arg);

void pool_init(int max_thread_num)
{
    pool = (Pthread_pool*)malloc(sizeof(Pthread_pool));   //为线程池申请堆上的内存
    pool ->  threadid_arr = (pthread_t*)malloc(sizeof(pthread_t) * max_thread_num);
    pthread_mutex_init(&(pool -> mutex), NULL);   //互斥锁的初始化
    pthread_cond_init(&(pool -> cond), NULL);
    pool -> list1 = NULL;
    pool -> max_num = max_thread_num;
    pool -> shutdown = 0;
    pool -> m_max_requests = 0;
    for(int i = 0; i < max_thread_num; i++)
    {
        pthread_create(&(pool -> threadid_arr[i]), NULL, Clib_Accept, NULL);
    }
}

void Pool_Add_Worker(void *(*Process) (void *arg), void* arg)
{
    Pthread_Worker *new_worker = (Pthread_Worker*)malloc(sizeof(Pthread_Worker));
    new_worker -> Process = Process;
    new_worker -> arg = arg;
    new_worker -> next = NULL;
    pthread_mutex_lock(&(pool -> mutex));   //对任务队列加锁
    //使用条件变量

    /*添加任务到任务队列中*/
    Pthread_Worker *worker1 = pool -> list1;
    if(worker1 != NULL)
    {
        while(worker1 != NULL)
            worker1 = worker1 -> next;
        worker1 -> next = new_worker;
    }
    else
        pool -> list1 = new_worker;

    pool -> list1_num++;
    pthread_mutex_unlock(&(pool -> mutex));
    pthread_cond_signal(&(pool -> cond));
}


/*从任务队列中选取任务进行跑*/
void *Pthread_run(void* arg)
{
    while(1)
    {
        pthread_mutex_lock(&(pool -> mutex));
        while(pool -> list1_num == 0 && !(pool -> shutdown))
        {
            pthread_cond_wait(&(pool -> cond), &(pool -> mutex));
        }

        //线程池销毁
        if(pool -> shutdown)
        {
            pthread_mutex_unlock(&(pool -> mutex));
            pthread_exit(NULL);
        }
        if(pool -> list1_num == 0 && pool -> list1 == NULL)
            return NULL;

        //将列表的头任务取出来并且将任务的数目➖1
        Pthread_Worker *worker = pool -> list1 -> next;
        pool -> list1 -> next = worker -> next;
        pool -> list1_num--;
        pthread_mutex_unlock(&(pool -> mutex));

        //调用回调函数
        (*(worker -> Process))(worker -> arg);
        free(worker);
        worker = NULL;
    }
}


void* Clib_Accept(void *arg)
{
    sock* mythis = (sock*)arg;
    while(1)
    {
        char buff[256] = {0};
        //接收客户端消息
        int n = recv(mythis->c, buff, 128, 0);
        if(n <= 0)
        {
            //关闭连接
            close(mythis->c);
            break;
        }
        printf("buff: %s\n", buff);
        //向客户端发送消息  "👌"，接收成功
        send(mythis->c, "ok", 2, 0);
    }
    close(mythis->sockfd);
}


void main()
{
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);  //创建套接字
    if(-1 == sockfd)
        return;

    struct sockaddr_in ser,cli;     //在绑定函数(bind();)中需要的结构体，用来记录客户端的ip地址和端口号
    ser.sin_family = PF_INET;       //地址族：Tcp/Ip
    ser.sin_port = htons(6000);     //将客户端的端口号转化为网络字节序
    ser.sin_addr.s_addr = inet_addr("127.0.0.1"); //将客户端的ip地址转化为网络字节序  "127.0.0.1"是连接本机的ip地址

    int ret = bind(sockfd, (struct sockaddr*)&ser, sizeof(ser));   //命名套接字
    if(-1 == ret)
        return;

    int listen_fd = listen(sockfd, 5);
    if(-1 == listen_fd)
        return;
    int len = sizeof(cli);
    //连接套接字  通过c实现服务器与客户端之间的通信
    while(1)
    {
        int c = accept(sockfd, (struct sockaddr*)&cli, &len);
        if(-1 == c)
            return;
        sock *workingname = (sock*)malloc(sizeof(sock));
        workingname->c = c;
        workingname->sockfd = sockfd;
        Pool_Add_Worker(Clib_Accept, workingname);
        free(workingname);
    }
}
#endif







