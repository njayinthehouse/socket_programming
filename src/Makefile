
client : client.cpp client_utils.c client_utils.h
	g++ client.cpp client_utils.c client_utils.h -o client -g -std=c++17 -pthread -O2

server : server.cpp server_utils.c server_utils.h chatroom_utils.cpp chatroom_utils.hpp
	g++ server.cpp server_utils.c server_utils.h chatroom_utils.cpp chatroom_utils.hpp -o server -g -std=c++17 -pthread -O2

all : client server

clean :
	rm server client
