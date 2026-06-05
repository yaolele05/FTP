CXX := g++
CXXFLAGS := -std=c++17 -Wall -g

# 服务端全部源码
SERVER_SRC = FTPserver/mainserver.cpp $(wildcard FTPserver/net/*.cpp) $(wildcard FTPserver/ftp/*.cpp)
# 客户端全部源码
CLIENT_SRC = FTPclient/mainclient.cpp FTPclient/ftpclient.cpp

all: server client

server: $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $(SERVER_SRC) -o server

client: $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRC) -o client

clean:
	rm -f server client
