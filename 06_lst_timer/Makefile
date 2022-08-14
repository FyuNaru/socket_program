all:server client

SRC_PATH = ../src

server:06_server.cpp 
	g++ 06_server.cpp -o server

client:06_client.cpp $(SRC_PATH)/TcpClient.cpp
	g++ 06_client.cpp $(SRC_PATH)/TcpClient.cpp -I $(SRC_PATH) -o client


clean:
	rm server client