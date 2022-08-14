#include <sys/wait.h>
#include <signal.h>
#include <sys/epoll.h>
#include "lst_timer.h"

#include <iostream>
#include <stdlib.h>
//包含各种关于socket的函数
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
//包含IP地址字符串与网络字节序无符号整数互转的函数：inet_pton(), inet_ntop
#include <arpa/inet.h>
//包含主机字节序与网络字节序互转的函数：htonl, htons, ntohl, ntohs
#include <netinet/in.h>
//包含close
#include <unistd.h>
#include <string>
//包含perror
#include <errno.h>
//包含fcntl
#include <fcntl.h>
//包含信号相关
#include <signal.h>
//
#include <assert.h>


//定义缓冲区大小，测试lt et时可以设置为一个比较小的值
#define BUFF_SIZE 10
//定义一次最多读取的epoll event
#define MAX_EPOLL_EVENT 1024
//打开的客户端连接fd上限
#define FD_LIMIT 65535

//设置时钟周期为5秒，即五秒检查一次有无空闲连接
#define TIMESLOT 5

//https://blog.csdn.net/weixin_43222324/article/details/106989714
//全局变量和静态变量的区别：二者都分配在全局区，区别主要是能否被外界文件所见。
//创建全双工管道
static int pipefd[2];
//创建升序定时器链表
static sort_timer_lst timer_lst;

using namespace std;

//设置为全局变量时调用exit(0)可以自动调用析构函数释放资源
// TcpServer tcpServer;
//测试lt
int lt_count = 0;
//控制终止服务器
bool stop_server = false;
//控制计时器是否已经到时
bool timeout = false;
//设置epfd为全局变量
int epfd = -1;

//将指定的文件描述符设置为非阻塞的，P113有详细的解释
int setnonblocking( int fd )
{   
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

//每个计时器用到的到时处理函数
void cb_func( client_data* user_data )
{
    epoll_ctl( epfd, EPOLL_CTL_DEL, user_data->sockfd, 0 );
    assert( user_data );
    close( user_data->sockfd );
}

//超时处理函数
void timer_handler()
{
    timer_lst.tick();
    //重新计时，因为每个alarm只能定一次时
    alarm( TIMESLOT );
}

//参考统一事件源
void sig_handler( int sig )
{
    int save_errno = errno;
    int msg = sig;
    send( pipefd[1], ( char* )&msg, 1, 0 );
    errno = save_errno;
}

//参考统一事件源
void addsig( int sig )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

//为指定的fd在epfd上注册一个指定的event，是否选择ET模式要额外设定
void addfd(int epfd, int fd, int event, bool isET){
    epoll_event ep_event;
    memset(&ep_event, 0, sizeof(ep_event));
    ep_event.events = event;
    ep_event.data.fd = fd;
    if(isET){
        ep_event.events |= EPOLLET;
        //未完成：如果是ET模式，还要将指定的文件描述符设置为非阻塞的

    }
    //将该事件加入到监听事件表
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ep_event) != 0){
        cout << "error: epoll_ctl" << endl;
    }
}

//lt模式处理事件
//需要用到epfd：为新连接上的客户添加新的事件
//需要用到listenfd：比较某个发生事件的fd是否为listenfd
//修改为了不需要用到TcpServer类
void lt(int event_count, epoll_event * event_list, int epfd, int listenfd, client_data * users){
    for(int i=0; i<event_count; i++){
        //获取对应事件的fd
        int event_fd = event_list[i].data.fd;
        if(event_fd == listenfd){
            //server fd上有事件，即有新的连接：打开socket，为客户增加一个监听的event
            if((event_list[i].events & EPOLLIN) == 1){
                
                /* ************************************* */
                //接受一个链接, accept返回-1表示失败
                struct sockaddr_in client_socket_addr;
                //client_socket_len必须初始化否则accept会出错
                socklen_t client_socket_len = sizeof(struct sockaddr_in);
                int client_fd = accept(listenfd, (struct sockaddr *)&client_socket_addr, (socklen_t*)&client_socket_len);
                if(client_fd < 0){
                    cout << "error: accept:" << strerror(errno) << endl;
                    return;
                } else {
                    cout << "与客户端: " << "(ip: " << inet_ntoa(client_socket_addr.sin_addr);
                    cout << ", port: " << ntohs(client_socket_addr.sin_port) << ") 建立了连接" << endl;
                }
                /* ************************************* */
                addfd(epfd, client_fd, EPOLLIN, false);
                /* ************************************* */
                //为新链接上的用户创建定时器
                users[client_fd].address = client_socket_addr;
                users[client_fd].sockfd = client_fd;
                util_timer * timer = new util_timer;
                timer->user_data = &users[client_fd];
                timer->cb_func = cb_func;
                //预定的超时时间为当前时间加上三个周期后
                timer->expire = time(nullptr) + 3 * TIMESLOT;
                users[client_fd].timer = timer;
                //将该新生成的定时器加入到升序链表中的合适位置
                timer_lst.add_timer(timer);
                /* ************************************* */
            } else {
                cout << "未知事件发生" << endl;
            }
        } else if((event_fd == pipefd[0]) && ( event_list[i].events & EPOLLIN )){
            //事件是pipe[0]上的，并且必须是pipe0上的读事件，说明有信号发生
            //处理信号
            int sig;
            char signals[1024];
            int ret = recv( pipefd[0], signals, sizeof( signals ), 0 );
            if( ret == -1 ) {
                // handle the error
                continue;
            } else if( ret == 0 ){
                continue;
            } else {
                //ret的长度即为信号的个数
                for( int i = 0; i < ret; ++i ){
                    switch( signals[i] ){
                        case SIGALRM:{
                            //定时器该触发了。但是需要先将其他所有已经发生的I/O事件处理完
                            //再处理定时器的事件
                            timeout = true;
                            break;
                        }
                        case SIGTERM:{
                            stop_server = true;
                            break;
                        }
                        case SIGINT:{
                            stop_server = true;
                        }
                    }
                }
            }
        } else if((event_list[i].events & EPOLLIN) == 1){
            //某个客户端上有事件（包括数据和断开）
            char buffer[BUFF_SIZE];
            //测试LT模式：当一次事件未被处理完（例如数据大于BUFF_SIZE能读取的数据，
            //或者本轮放弃处理该事件（并不用recv将缓存中的数据读出来））
            //正确的表现为：未被处理完的事件重新加回到发生的事件集合中，下次调用epoll_wait时重新产生
            // cout << "第 " << lt_count << " 轮触发 " << event_fd << " 上的事件" << endl;
            memset(buffer, '\0', BUFF_SIZE);
            ssize_t recv_len = recv(event_fd, buffer, BUFF_SIZE-1, 0);
            //获取该用户的定时器索引，方便操作。
            util_timer* timer = users[event_fd].timer;
            if(recv_len <= 0){
                //recv返回-1表示出错，返回0表示连接已经断开
                if(recv_len == 0){
                    //复用cb函数关闭链接
                    cb_func(&users[event_fd]);
                    //用户断开链接时，要将定时器删除掉
                    if(timer){
                        timer_lst.del_timer(timer);
                    }
                    cout << "连接client_fd(" << event_fd << ")断开" << endl;
                } else {
                    //这段目前不太懂
                    if( errno != EAGAIN ){
                        //复用cb函数关闭链接
                        cb_func(&users[event_fd]);
                        //用户断开链接时，要将定时器删除掉
                        if(timer){
                            timer_lst.del_timer(timer);
                        }
                    }
                    cout << "error : recv :" << strerror(errno) << endl;
                }
                //这里将关闭用户fd操作合并到时钟到期的处理函数中，当然在这里操作也是没问题的
                //close(event_fd);
            } else{
                //当确实有事件发生时，更新该用户的定时器
                if(timer){
                    timer->expire = time(nullptr) + 3 * TIMESLOT;
                    timer_lst.adjust_timer(timer);
                }
                cout << "从client_fd(" << event_fd << ")接收到数据：" << buffer << endl;
            }
        } else {
            cout << "未知事件发生" << endl;
        }     
    }
    //处理完所有的I/O事件后，再来处理超时信号
    if( timeout ){
        // cout << "成功触发一次alarm" << endl;
        timer_handler();
        timeout = false;
    }
}

int main(int argc, char ** argv){
    //1.服务端建立，要求输入端口号
    if(argc <= 1){
        cout << "Using: " << argv[0] << " port" << endl;
        return -1;
    }
    //1.1提取参数信息
    int port = atoi(argv[1]);
    //3.初始化并监听
    //使用指定port和本机任意ip创建地址
    struct sockaddr_in server_socket_addr;
    bzero(&server_socket_addr, sizeof(server_socket_addr));
    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_port = htons(port);
    server_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    //绑定地址与socket，bind返回0表示绑定成功
    if(bind(server_fd, (struct sockaddr *)&server_socket_addr, sizeof(server_socket_addr)) != 0){
        cout << "error: bind:" << strerror(errno) << endl;
        close(server_fd);
        return 0;
    }
    //进入listen状态，listen返回0表示成功
    if(listen(server_fd, 5) != 0){
        cout << "error: listen:" << strerror(errno) << endl;
        close(server_fd);
        return 0;
    }


    //创建epoll fd，参数为任意正数即可
    epfd = epoll_create(1);
    //为server fd创建要监听的事件
    addfd(epfd, server_fd, EPOLLIN, false);

    //创建一个全双工管道，在0端用epoll监听事件，有alarm信号的时候往1端写入事件
    //1要设置为非阻塞的，如果缓冲区满，不能使其阻塞在send那里。
    //实现信号统一事件源
    int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, pipefd );
    assert( ret != -1 );
    setnonblocking( pipefd[1] );
    addfd( epfd, pipefd[0], EPOLLIN, false);

    //添加信号的统一事件源
    addsig( SIGALRM );
    addsig( SIGTERM );
    addsig( SIGINT );
    //创建定时器中每个定时器对应的用户数据
    client_data* users = new client_data[FD_LIMIT]; 
    //创建一个用于保存发生的event的数组
    epoll_event ep_event_list[MAX_EPOLL_EVENT];

    //开始定时器定时
    alarm( TIMESLOT );
    while(!stop_server){
        //开始epoll wait, timeout 设置为-1表示持续阻塞到有事件发生
        int event_count = epoll_wait(epfd, ep_event_list, MAX_EPOLL_EVENT, -1);
        
        //这里后半段条件必须加，该条件的意思是：当alarm触发信号时，epoll_wait会被更高级的打断
        //并且将错误号设置为EINTR。因此我们必须忽略掉alarm产生的打断
        //如果不加这个条件，那么等alarm触发时，epoll wait会被打断，错误信息：Interrupted system call
        if((event_count) < 0 && ( errno != EINTR )){
            cout << "error: epoll_wait" << strerror(errno) << endl;
            break;
        }
        //使用LT模式
        lt_count++;
        lt(event_count, ep_event_list, epfd, server_fd, users);
    }
    
    //6.退出程序时自动调用析构函数释放socket
    cout << "父进程" << getpid() << "退出" << endl;
    close(server_fd);
    close(pipefd[0]);
    close(pipefd[1]);
    delete [] users;
    return 0;
}