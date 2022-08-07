## readme

### 文件说明

#### 01_tcp_server_client

* `01_tcp_client.cpp` : 一个简单的tcp客户端程序，向指定的服务器发送多条数据。

* `01_tcp_server.cpp` : 一个简单的tcp服务器，从客户端接收数据，并打印出来。

#### 02_test_class

* `02_test_class_server.cpp` : 一个简单的tcp服务器，使用封装的类

* `02_test_class_client.cpp` : 一个简单的tcp客户端，使用封装的类

#### 03_multi_progress

实现了一个基本的多进程服务器，可以使用多进程的方式处理进程

#### 04_select

实现了一个基本的使用select实现I/O复用的服务器
测试一：可以处理大量的请求（客户端测试为发送一百万条数据）
测试二：测试连接多个客户端时socket编号是否正确

#### 05_epoll

实现了基本的epoll代码（功能类似于select，一个服务器可同时处理多个客户端的事件），使用lt模式并进行了测试。

需要注意TcpServer这个类过于基础，不适合I/O复用中多个客户端的连接（主要是client_fd被更新后容易出错）只适用于一对一连接或者多进程代码。