#include "client_utils.h"
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
using namespace std;

class safebuffer {
    vector<string> data;
    mutex mtx;

    public:
    vector<string> flush() {
        lock_guard<mutex> lock(mtx);
        auto d = data;
        data.clear();
        return d;
    }

    void write(string s) {
        if (s.compare("") != 0) {
            lock_guard<mutex> lock(mtx);
            data.push_back(s);
        }
    }
};

void flush_buffer(int sockfd, safebuffer* buf) {
    for (auto s : buf->flush()) {
        if (s.empty()) 
            continue;
        else if (s.at(0) == '*') {
            int pos = s.find('|');
            cout << s.substr(pos + 1) << '\n';
            send(sockfd, s.substr(1, pos - 1).c_str(), 10000, 0);
        }
        else if (s.at(0) == '@') {
            ifstream is(s.substr(1));
            stringstream buffer;
            buffer << is.rdbuf();
            send(sockfd, buffer.str().c_str(), 1000001, 0); 
        }
        else if (s.at(0) == '+') {
            int pos = s.find('|');
            ofstream os(s.substr(pos + 1));
            os << s.substr(1, pos - 1);
            os.close();
        }
        else {
            cerr << "FLUSHING?";
            cout << s << '\n';
        }
    }
}

void spear(int sockfd, safebuffer* buf) {
    string s;
    cerr << "(Spearing)\n";
    while (true) {
        usleep(10000);
        flush_buffer(sockfd, buf);
        cout << ">> ";
        getline(cin, s, '\n'); 
        send(sockfd, s.c_str(), 10000, 0);
        cerr << "Sent " << s << '\n';
    }
}

void shield(int sockfd, safebuffer* buf) {
    char b[10000];
    cerr << "(Shielding)";

    while (true) {
        if (recv(sockfd, b, 10000, 0) == -1) {
            perror("recv");
            exit(1);
        }
        buf->write(string(b));
    }
}

void run_client(int sockfd) {
    safebuffer *buf = new safebuffer();
    thread sender(spear, sockfd, buf);
    thread receiver(shield, sockfd, buf);

    sender.join();
    receiver.join();
    close(sockfd);
}

int main() {
    char host[25];
    strcpy(host, "localhost");
    run_client(client(host));
    return 0;
}
