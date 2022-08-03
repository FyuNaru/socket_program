#include "TcpClient.h"

using namespace std;

TcpClient::TcpClient(){
    if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        cout << "error: socket: " << strerror(errno) << endl;
    }
}

TcpClient::~TcpClient(){
    close(client_fd);
}

bool TcpClient::Connect(const char * server_name, int server_port){
    //服务端地址创建
    struct sockaddr_in server_socket_addr;
    bzero(&server_socket_addr, sizeof(server_socket_addr));
    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_port = htons(server_port);
    struct hostent * server_info;
    if((server_info = gethostbyname(server_name)) == nullptr){
        cout << "error: gethostbyname " << strerror(errno) << endl;
        return false;
    }
    memcpy(&server_socket_addr.sin_addr.s_addr, server_info->h_addr_list[0], server_info->h_length);
    //主动连接到服务器,connect返回-1表示不成功
    if(connect(client_fd, (struct sockaddr *)&server_socket_addr, sizeof(server_socket_addr)) != 0){
        cout << "error: connect :" << strerror(errno) << endl;
        return false;
    }
    return true;
}

bool TcpClient::Send(const char * buffer, int buflen){
    ssize_t send_len = send(client_fd, buffer, buflen, 0);
    if(send_len == -1){
        cout << "error: send :" << strerror(errno) << endl;
        return false;
    }
    return true;
}
bool TcpClient::Recv(char * buffer, int buflen){
    ssize_t recv_len = recv(client_fd, buffer, buflen, 0);
    if(recv_len <= 0){
        //recv返回-1表示出错，返回0表示连接已经断开
        if(recv_len == 0){
            cout << "连接断开" << endl;
        } else {
            cout << "error : recv :" << strerror(errno) << endl;
        }
        close(client_fd);
        return false;
    }
    return true;
}