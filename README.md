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