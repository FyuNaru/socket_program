//01
//一个简单的tcp客户端程序，向指定的服务器发送多条数据。
//1.可以观察到程序运行结果：由于tcp连接是流式的，发送的多条报文如果长度比较短（小于接收端buffer）则会在极短时间内一起到达服务端
//并且统一被储存在缓存中，最后一起打印出来。
//但是如果网速慢的话可能就分成了多次
//2.gethostbyname对于域名，本地ip，外网ip都是可以用的

#include <iostream>
#include <stdlib.h>
//包含addr_in
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
//包含IP地址字符串与网络字节序无符号整数互转的函数：inet_pton(), inet_ntop
#include <arpa/inet.h>
//包含主机字节序与网络字节序互转的函数：htonl, htons, ntohl, ntohs
#include <netinet/in.h>
//包含gethostbyname
#include <netdb.h>
//包含close, sleep
#include <unistd.h>
//包含perror
#include <errno.h>

//定义缓冲区大小
#define BUFF_SIZE 1024

using namespace std;

int main(int argc, char**argv){
    //1.客户端建立，要求输入主机地址，主机端口号
    //注意客户端程序端口号是自动分配的
    if(argc <= 2){
        cout << "Using: " << argv[0] << " server_ip server_port" << endl;
        return -1;
    }
    char * server_name = argv[1];
    int server_port = atoi(argv[2]);

    //2.服务端地址创建
    struct sockaddr_in server_socket_addr;
    bzero(&server_socket_addr, sizeof(server_socket_addr));
    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_port = htons(server_port);
    struct hostent * server_info;
    if((server_info = gethostbyname(server_name)) == nullptr){
        cout << "error: gethostbyname " << server_name << endl;
        return -1;
    }
    //简单选取第一个ip作为服务端ip。（返回的是字符串格式的网络字节序的ip，可直接以复制内存的形式使用）
    memcpy(&server_socket_addr.sin_addr.s_addr, server_info->h_addr_list[0], server_info->h_length);
    //3.创建socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0){
        cout << "error" << endl;
        return -1;
    }
    //4.主动连接到服务器,connect返回-1表示不成功
    if(connect(client_fd, (struct sockaddr *)&server_socket_addr, sizeof(server_socket_addr)) != 0){
        cout << "error" << endl;
        return -1;
    }
    //发送一些报文
    for(int i=0; i<5; i++){
        //注意应该是const常量字符串
        const char * buffer = "this is a message.";
        send(client_fd, buffer, strlen(buffer), 0);
        cout << "发送了一个报文：" << buffer << endl;
    }

    close(client_fd);
    return 0;
}