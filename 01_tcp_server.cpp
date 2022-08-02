//01
//一个简单的tcp服务器，从客户端接收数据，并打印出来

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

//定义缓冲区大小
#define BUFF_SIZE 1024

using namespace std;

int main(int argc, char**argv){
    //1.服务端建立，要求输入端口号
    if(argc <= 1){
        cout << "Using: " << argv[0] << " port" << endl;
        return -1;
    }
    //1.1提取参数信息
    int port = atoi(argv[1]);

    //2.客户端建立socket
    //2.1创建地址
    struct sockaddr_in server_socket_addr;
    bzero(&server_socket_addr, sizeof(server_socket_addr));
    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_port = htons(port);
    //地址中ip的处理
    //（1）使用本机对应的任意ip时：使用htonl
    //（2）使用参数中输入的ip地址时：使用server_socket_addr.sin_addr.s_addr = inet_addr("127.0.0.1")
    //（3）使用参数中输入的ip地址时：使用inet_pton(AF_INET, 源指针(argv[1])，目标指针(&server_socket_addr.sin_addr))
    server_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //2.2创建socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    //2.3绑定地址与socket，bind返回0表示绑定成功
    if(bind(server_fd, (struct sockaddr *)&server_socket_addr, sizeof(server_socket_addr)) != 0){
        cout << "error: bind" << endl;
        close(server_fd);
        return -1;
    }
    //3.进入listen状态，listen返回0表示成功
    if(listen(server_fd, 5) != 0){
        cout << "error: listen" << endl;
        close(server_fd);
        return -1;
    }
    //4.接受一个链接, accept返回-1表示失败
    struct sockaddr_in client_socket_addr;
    //client_socket_len必须初始化否则accept会出错
    socklen_t client_socket_len = sizeof(struct sockaddr_in);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_socket_addr, (socklen_t*)&client_socket_len);
    if(client_fd < 0){
        cout << "error: accept:" << strerror(errno) << endl;
        close(server_fd);
        return -1;
    } else {
        cout << "与客户端: " << "(ip: " << inet_ntoa(client_socket_addr.sin_addr);
        cout << ", port: " << ntohs(client_socket_addr.sin_port) << ") 建立了连接" << endl;
    }
    //5.从该连接接受数据
    char buffer[BUFF_SIZE];
    while(true){
        memset(buffer, '\0', BUFF_SIZE);
        //这里是想将接收到的数据用字符串形式展示出来，所以先用memset初始化为'\0‘，再接收BUFF_SIZE-1大小的数据
        //从而可以直接打印出接收到的字符串
        //返回值为实际接收的数据长度
        ssize_t recv_len = recv(client_fd, buffer, BUFF_SIZE-1, 0);
        if(recv_len <= 0){
            //recv返回-1表示出错，返回0表示连接已经断开
            if(recv_len == 0){
                cout << "与客户端: " << "(ip: " << inet_ntoa(client_socket_addr.sin_addr);
                cout << ", port: " << ntohs(client_socket_addr.sin_port) << ") 连接断开" << endl;
            } else {
                cout << "error " << endl;
            }
            close(client_fd);
            break;
        }
        cout << "从客户端: (ip: " << inet_ntoa(client_socket_addr.sin_addr);
        cout << ", port: " << ntohs(client_socket_addr.sin_port) << ")";
        cout << "接收到数据：" << buffer << endl;
    }
    close(server_fd);
    return 0;
}