all:server client

SRC_PATH = ../src

server:05_server.cpp $(SRC_PATH)/TcpServer.cpp
	g++ 05_server.cpp $(SRC_PATH)/TcpServer.cpp -I $(SRC_PATH) -o server

client:05_client.cpp $(SRC_PATH)/TcpClient.cpp
	g++ 05_client.cpp $(SRC_PATH)/TcpClient.cpp -I $(SRC_PATH) -o client

clean:
	rm server client