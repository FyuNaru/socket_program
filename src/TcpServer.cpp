#include "TcpServer.h"

using namespace std;

TcpServer::TcpServer(){
    //== 优先级比 = 高
    if((this->server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        cout << "error: socket: " << strerror(errno) << endl;
    }
}

TcpServer::~TcpServer(){
    close(server_fd);
    close(client_fd);
    cout << "自动释放" << endl;
}

bool TcpServer::InitServer(int port){
    if(this->server_fd == -1){
        //未处理错误
        return false;
    }
    //使用指定port和本机任意ip创建地址
    struct sockaddr_in server_socket_addr;
    bzero(&server_socket_addr, sizeof(server_socket_addr));
    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_port = htons(port);
    server_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //绑定地址与socket，bind返回0表示绑定成功
    if(bind(server_fd, (struct sockaddr *)&server_socket_addr, sizeof(server_socket_addr)) != 0){
        cout << "error: bind:" << strerror(errno) << endl;
        close(server_fd);
        return false;
    }
    //进入listen状态，listen返回0表示成功
    if(listen(server_fd, 5) != 0){
        cout << "error: listen:" << strerror(errno) << endl;
        close(server_fd);
        return false;
    }
    return true;
}

bool TcpServer::Accept(){
    //接受一个链接, accept返回-1表示失败
    struct sockaddr_in client_socket_addr;
    //client_socket_len必须初始化否则accept会出错
    socklen_t client_socket_len = sizeof(struct sockaddr_in);
    this->client_fd = accept(server_fd, (struct sockaddr *)&client_socket_addr, (socklen_t*)&client_socket_len);
    if(client_fd < 0){
        cout << "error: accept:" << strerror(errno) << endl;
        close(server_fd);
        return false;
    } else {
        cout << "与客户端: " << "(ip: " << inet_ntoa(client_socket_addr.sin_addr);
        cout << ", port: " << ntohs(client_socket_addr.sin_port) << ") 建立了连接" << endl;
        return true;
    }
}

bool TcpServer::Send(const char * buffer, int buflen){
    ssize_t send_len = send(client_fd, buffer, buflen, 0);
    if(send_len == -1){
        cout << "error: send :" << strerror(errno) << endl;
        return false;
    }
    return true;
}

bool TcpServer::Recv(char * buffer, int buflen){
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

void TcpServer::CloseClient(){
    close(client_fd);
}

void TcpServer::CloseListen(){
    close(server_fd);
}