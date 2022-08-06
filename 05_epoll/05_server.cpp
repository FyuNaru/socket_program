#include "TcpServer.h"
#include <sys/wait.h>
#include <signal.h>
#include <sys/epoll.h>

//定义缓冲区大小，测试lt et时可以设置为一个比较小的值
#define BUFF_SIZE 10
//定义一次最多读取的epoll event
#define MAX_EPOLL_EVENT 1024

using namespace std;

//设置为全局变量时调用exit(0)可以自动调用析构函数释放资源
TcpServer tcpServer;
//测试lt
int lt_count = 0;

//参考自13.3
//子进程完成任务后正常结束时的信号处理函数
static void handle_child(int sig){
    pid_t pid;
    int stat;
    //waitpid设置为非阻塞
    //当还没有结束时返回0，结束while循环；当某个进程结束时，返回子进程号，并处理
    //-1表示等待任意的子进程结束
    while( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
        cout << "子进程：" << pid << "退出" << endl;
    }
}

//发送结束信号时父进程的信号处理函数
//注意这种非正常的退出并不会调用局部的对象的析构函数
//参考：https://blog.csdn.net/Erice_s/article/details/117385637
//虽然不用担心内存泄漏，但是由于socket等没有被关闭，因此连接不会立即释放，netstat查看可得
static void handle_father_exit(int sig){
    //防止在调用该处理函数的过程中又接收到信号
    if(sig > 0){
        signal(SIGINT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);
    }

    //先关闭所有的子进程，再关闭自己
    //0表示向所有子进程发送信号，包括自己（但是自己在一开始屏蔽了15这个信号)
    kill(0, 15);
    cout << "父进程" << getpid() << "退出" << endl;
    exit(0);
}

static void handle_child_exit(int sig){
    //防止在调用该处理函数的过程中又接收到信号
    if(sig > 0){
        signal(SIGINT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);
    }
    cout << "子进程" << getpid() << "退出" << endl;
    exit(0);
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

//lt模式
//需要用到epfd：为新连接上的客户添加新的事件
//需要用到listenfd：比较某个发生事件的fd是否为listenfd
//需要用到TcpServer类
void lt(int event_count, epoll_event * event_list, int epfd){
    for(int i=0; i<event_count; i++){
        //获取对应事件的fd
        int event_fd = event_list[i].data.fd;
        if(event_fd == tcpServer.server_fd){
            //server fd上有事件，即有新的连接：打开socket，为客户增加一个监听的event
            if((event_list[i].events & EPOLLIN) == 1){
                tcpServer.Accept();
                int client_fd = tcpServer.client_fd;
                addfd(epfd, client_fd, EPOLLIN, false);
            } else {
                cout << "未知事件发生" << endl;
            }
        } else if((event_list[i].events & EPOLLIN) == 1){
            //某个客户端上有事件（包括数据和断开）
            char buffer[BUFF_SIZE];
            //测试LT模式：当一次事件未被处理完（例如数据大于BUFF_SIZE能读取的数据，
            //或者本轮放弃处理该事件（并不用recv将缓存中的数据读出来））
            //正确的表现为：未被处理完的事件重新加回到发生的事件集合中，下次调用epoll_wait时重新产生
            cout << "第 " << lt_count << " 轮触发 " << event_fd << " 上的事件" << endl;
            memset(buffer, '\0', BUFF_SIZE);
            if(tcpServer.Recv(buffer, BUFF_SIZE-1) == false){
                //发生了断开：关闭socket
                //貌似不用手动删除event，关闭fd后对应的事件都会被删除
                //注意这里一定要关指定的fd，不能直接调用tcpServer的fd，会关错fd导致死循环
                //例如有5 6两个fd，5先结束，关闭5时不小心关掉了6.会导致5还残留一个关闭的事件没有解决
                //会无限循环，不停的在recv处出错。
                close(event_fd);
                cout << "关闭" << event_fd << endl;
            }
            else{
                cout << "从client_fd(" << event_fd << ")接收到数据：" << buffer << endl;
            }
        } else {
            cout << "未知事件发生" << endl;
        }     
    }
}

int main(int argc, char ** argv){
    //忽略所有的信号
    for(int i=0; i<=64; i++){
        signal(i, SIG_IGN);
    }
    //对子进程结束的信号处理，防止产生僵尸进程
    signal(SIGCHLD, handle_child);
    //对于ctrlc和kill信号，做退出处理
    signal(SIGINT, handle_father_exit);
    signal(SIGTERM, handle_father_exit);

    //1.服务端建立，要求输入端口号
    if(argc <= 1){
        cout << "Using: " << argv[0] << " port" << endl;
        return -1;
    }
    //1.1提取参数信息
    int port = atoi(argv[1]);
    //3.初始化并监听
    if(tcpServer.InitServer(port) == false){
        return 0;
    }

    //创建epoll fd，参数为任意正数即可
    int epfd = epoll_create(1);
    //为server fd创建要监听的事件
    addfd(epfd, tcpServer.server_fd, EPOLLIN, false);

    //创建一个用于保存发生的event的数组
    epoll_event ep_event_list[MAX_EPOLL_EVENT];
    while(true){
        //开始epoll wait, timeout 设置为-1表示持续阻塞到有事件发生
        int event_count = epoll_wait(epfd, ep_event_list, MAX_EPOLL_EVENT, -1);
        if(event_count < 0){
            cout << "error: epoll_wait" << endl;
            break;
        }
        //使用LT模式
        lt_count++;
        lt(event_count, ep_event_list, epfd);
    }
    
    //6.退出程序时自动调用析构函数释放socket
    return 0;
}