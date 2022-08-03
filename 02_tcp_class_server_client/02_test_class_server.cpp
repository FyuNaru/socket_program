#include "TcpServer.h"

//定义缓冲区大小
#define BUFF_SIZE 1024

using namespace std;

int main(int argc, char ** argv){
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
    //4.从监听队列取出一个
    if(tcpServer.Accept() == 0){
        return 0;
    }
    //5.从该连接接受数据
    char buffer[BUFF_SIZE];
    while(true){
        memset(buffer, '\0', BUFF_SIZE);
        if(tcpServer.Recv(buffer, BUFF_SIZE-1) == false){
            break;
        }
        cout << "接收到数据：" << buffer << endl;
    }
    //6.退出程序时自动调用析构函数释放socket
    return 0;
}