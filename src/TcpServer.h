#ifndef TCP_SERVER_H
#define TCP_SERVER_H

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

using namespace std;

class TcpServer{
public:
    //服务端监听socket
    int server_fd = -1;
    //客户端连接socket，目前只连接一个
    int client_fd = -1;
    //构造函数
    TcpServer();
    //
    ~TcpServer();

    //使用指定的端口号绑定socket，并使其处于监听状态。返回bind和listen结果
    bool InitServer(int port);
    //阻塞等待连接，返回accept结果
    bool Accept();

    //发送报文
    bool Send(const char * buffer, int buflen);
    //接收报文
    bool Recv(char * buffer, int buflen);
};

#endif