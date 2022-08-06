#include "TcpServer.h"
#include <sys/wait.h>
#include <signal.h>
#include <sys/select.h>

//定义缓冲区大小
#define BUFF_SIZE 1024

using namespace std;

//设置为全局变量时调用exit(0)可以自动调用析构函数释放资源
TcpServer tcpServer;

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

    // select使用
    //创建待read的fd集合
    fd_set read_fds;
    FD_ZERO(&read_fds);
    //将listenfd加入
    FD_SET(tcpServer.server_fd, &read_fds);
    int maxfd = tcpServer.server_fd + 1;


    while(true){
        //创建一个临时的fd_set，用于储存有事件发生并修改后的fd_set
        fd_set temp_fds = read_fds;
        //timeout为null时，一直阻塞直到有消息
        //第一个参数是要监听的fd总数，即范围：0-(maxfd-1)，监听范围内最大的fd为maxfd-1
        int event_count = select(maxfd, &temp_fds, nullptr, nullptr, nullptr);
        if(event_count < 0){
            //select 失败
            cout << "error: select" << endl;
            return 0;
        }
        //记录已经处理的socket的数量
        int deal_count = 0;
        for(int i=0; i<=maxfd; i++){
            //遍历查找是哪个fd准备就绪了
            if(FD_ISSET(i, &temp_fds)){
                deal_count++;
                if(i == tcpServer.server_fd){
                    //server fd上有事件，即有新的连接：打开socket，增加位图，更新maxfd
                    //4.从监听队列取出一个
                    tcpServer.Accept();
                    FD_SET(tcpServer.client_fd, &read_fds);
                    if(tcpServer.client_fd >= maxfd){
                        maxfd = tcpServer.client_fd + 1;
                    }
                }
                else{
                    //某个客户端上有事件（包括数据和断开）
                    tcpServer.client_fd = i;
                    //5.从该连接接受数据
                    char buffer[BUFF_SIZE];
                    memset(buffer, '\0', BUFF_SIZE);
                    if(tcpServer.Recv(buffer, BUFF_SIZE-1) == false){
                        //发生了断开：清空位图，更新maxfd，关闭socket
                        FD_CLR(i, &read_fds);
                        tcpServer.CloseClient();
                        //重新找maxfd最大值
                        if(i == maxfd - 1){
                            while(FD_ISSET(--maxfd, &read_fds)) break;
                            maxfd++;
                        }
                    }
                    else{
                        cout << "从client_fd(" << i << ")接收到数据：" << buffer << endl;
                    }
                }
            }
            if(deal_count == event_count){
                break;
            }
        }
    }
    
    //6.退出程序时自动调用析构函数释放socket
    return 0;
}