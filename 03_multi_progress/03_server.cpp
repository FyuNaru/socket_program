#include "TcpServer.h"
#include <sys/wait.h>
#include <signal.h>

//定义缓冲区大小
#define BUFF_SIZE 1024

using namespace std;

//参考自13.3
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

int main(int argc, char ** argv){
    //对子进程结束的信号处理，防止产生僵尸进程
    signal(SIGCHLD, handle_child);

    //1.服务端建立，要求输入端口号
    if(argc <= 1){
        cout << "Using: " << argv[0] << " port" << endl;
        return -1;
    }
    //1.1提取参数信息
    int port = atoi(argv[1]);
    //2.客户端建立socket
    TcpServer tcpServer;
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