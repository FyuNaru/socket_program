#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

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

class TcpClient{
public:
    int client_fd;
    TcpClient();
    ~TcpClient();

    bool Connect(const char * server_name, int server_port);
    //发送报文
    bool Send(const char * buffer, int buflen);
    //接收报文
    bool Recv(char * buffer, int buflen);

};

#endif