#ifndef CHATROOM_UTILS_H
#define CHATROOM_UTILS_H

#include <map>
#include <mutex>
#include <chrono>
#include <vector>
#include <iostream>

using namespace std;

enum command {
    CREATE, LIST_CHATS, JOIN, LEAVE, ADD, LIST_USERS, REPLY_M, REPLY_TCP, REPLY_UDP, NO_OP
};

class chatroom_dict {
    map<string, vector<string>> data;
    mutex mtx;

    public:
    chatroom_dict();

    map<string, int> names;

    void reg(string name, int sfd);

    int push(string key, string client, bool create = true);

    int remove(string key, string client);

    const vector<string>& at(string key); 

    vector<int> cons_at(string key);

    vector<string> keys();
};

struct repl {

    pair<command, string> read();

    string eval(command c, string arg);

    int print(string msg);

    void operator()(int con, string name, chatroom_dict *chatrooms) {
        this->con = con;
        this->chatrooms = chatrooms;
        this->room = "";
        this->name = name;
        this->chatrooms->reg(name, con);
        cout << name << '\n';
        
        int i = 0;
        while (1) {
            auto [ c , s ] = read();
            string e = eval(c, s);
            if (i % 2 == 0) print(e);
            i++;
        }
        //while (print (eval (read ())));
    }

    private:
    int con;
    string name;
    chatroom_dict *chatrooms;
    string room;

    string receive(int sock);

    vector<string> split(string str);

    string join(const vector<string>& ss, string delimiter = "\n"); 

    int print(string msg, int con); 

    void broadcast(const vector<int>& clients, string msg); 
};


#endif
