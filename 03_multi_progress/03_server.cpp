#include "TcpServer.h"
#include <sys/wait.h>
#include <signal.h>

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
    while(true){
        //4.从监听队列取出一个
        if(tcpServer.Accept() == false){
            continue;
        }
        //接收到一个连接后，为其开一个子进程
        if(fork() > 0){
            //父进程
            //此时父进程和子进程都打开了与子进程连接的文件描述符，这个文件描述符有两个引用
            //我们关掉父进程对其的引用
            tcpServer.CloseClient();
            continue;
        }
        //子进程
        //原理同上，关掉子进程对listen fd的引用
        tcpServer.CloseListen();
        //fork之后子进程的信号需要重新设置为子进程对应的处理函数
        //不重新设置的话调用的仍然是父进程的信号处理函数
        signal(SIGINT, handle_child_exit);
        signal(SIGTERM, handle_child_exit);
        //5.从该连接接受数据
        char buffer[BUFF_SIZE];
        while(true){
            memset(buffer, '\0', BUFF_SIZE);
            if(tcpServer.Recv(buffer, BUFF_SIZE-1) == false){
                break;
            }
            cout << "子进程: " << getpid() << "接收到数据：" << buffer << endl;
        }
        //子进程结束后退出
        break;
    }
    
    //6.退出程序时自动调用析构函数释放socket
    return 0;
}