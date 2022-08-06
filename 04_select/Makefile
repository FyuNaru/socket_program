all:server client

SRC_PATH = ../src

server:04_server.cpp $(SRC_PATH)/TcpServer.cpp
	g++ 04_server.cpp $(SRC_PATH)/TcpServer.cpp -I $(SRC_PATH) -o server

client:04_client.cpp $(SRC_PATH)/TcpClient.cpp
	g++ 04_client.cpp $(SRC_PATH)/TcpClient.cpp -I $(SRC_PATH) -o client

clean:
	rm server client