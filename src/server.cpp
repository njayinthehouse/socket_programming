#include "server_utils.h"
#include "chatroom_utils.hpp"
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <thread>

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((sockaddr_in*)sa)->sin_addr);
    }

    return &(((sockaddr_in6*)sa)->sin6_addr);
}

void run_server(int sockfd, chatroom_dict *chatrooms, int i = 0) {
    int new_fd;
    sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        run_server(sockfd, chatrooms);
    }

    inet_ntop(their_addr.ss_family, get_in_addr((sockaddr *)&their_addr), s, sizeof s);
    cout << "server: got connection from " << s << '\n';
    string name = string("USER-");
    name.append(to_string(i));

    // Handle connection
    thread(repl(), new_fd, name, chatrooms).detach();

    run_server(sockfd, chatrooms, i + 1);
}

int main() {
    run_server(server(), new chatroom_dict());
    return 0;
}
