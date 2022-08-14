#include "TcpClient.h"

//定义缓冲区大小
#define BUFF_SIZE 1024

using namespace std;

int main(int argc, char ** argv){
    //1.客户端建立，要求输入主机地址，主机端口号
    //注意客户端程序端口号是自动分配的
    if(argc <= 2){
        cout << "Using: " << argv[0] << " server_ip server_port" << endl;
        return -1;
    }
    char * server_name = argv[1];
    int server_port = atoi(argv[2]);
    //2.创建socket
    TcpClient tcpClient;
    //3.连接服务器
    if(tcpClient.Connect(server_name, server_port) == false){
        return 0;
    }
    //4.发送消息
    for(int i=0; i<5; i++){
        //注意应该是const常量字符串
        const char * buffer = "aaaaaaaaaabbbbbbbbbbccc";
        tcpClient.Send(buffer, strlen(buffer));
        cout << "发送了一个报文：" << buffer << endl;
        sleep(2);
    }
    //长时间睡眠导致被断开
    sleep(20);
    //尝试再发一次报文
    //然而从结果发现，如果服务器端将对应的fd关闭，客户这里仍然可以发送报文，只不过接收不到了
    //需要重新建立链接。至于发送的报文哪里去了这个目前不太懂
    //https://blog.csdn.net/qq_37253168/article/details/121522032
    //原因解答，事实上只可以再调用send一次，下次就会报告链接不可用
    const char * buffer = "xxxxxxx";
    if(tcpClient.Send(buffer, strlen(buffer))){
        cout << "发送了一个报文：" << buffer << endl;
    } else {
        cout << "链接已被关闭" << endl;
    }
    if(tcpClient.Send(buffer, strlen(buffer))){
        cout << "发送了一个报文：" << buffer << endl;
    } else {
        cout << "链接已被关闭" << endl;
    }
    //5.结束时自动关闭socket
    return 0;
}