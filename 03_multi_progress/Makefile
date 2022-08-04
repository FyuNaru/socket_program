all:server client

SERVER = 03_server.cpp
CLIENT = 03_client.cpp
SRC_PATH = ../src

server:$(SERVER) $(SRC_PATH)/TcpServer.cpp
	g++ $(SERVER) $(SRC_PATH)/TcpServer.cpp -I $(SRC_PATH) -o server

client:$(CLIENT) $(SRC_PATH)/TcpClient.cpp
	g++ $(CLIENT) $(SRC_PATH)/TcpClient.cpp -I $(SRC_PATH) -o client

# 02_test_class_server.o:02_test_class_server.cpp

# 02_test_class_client.o:02_test_class_client.cpp

# $(SRC_PATH)/TcpServer.o:$(SRC_PATH)/TcpServer.cpp
# 	g++ -c $(SRC_PATH)/TcpServer.cpp -I $(SRC_PATH)

# $(SRC_PATH)/TcpClient.o:$(SRC_PATH)/TcpClient.o
# 	g++ -c $(SRC_PATH)/TcpClient.cpp -I $(SRC_PATH)

clean:
	rm server client