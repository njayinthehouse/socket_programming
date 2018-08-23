#include "chatroom_utils.hpp"
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

bool exists(vector<string> v, string s) {
    cout << "Exists? " << s << '\n';
    for (auto i : v)
        if (s.compare(i) == 0)
            return true;

    return false;
}

string read_file(string fname) {
    ifstream is(fname);
    stringstream buffer;
    buffer << is.rdbuf();
    return buffer.str();
}

void del(vector<string>& v, string s) {
    for (auto it = v.begin(); it != v.end(); it++) 
        if (s.compare(*it) == 0) {
            v.erase(it);
            cout << "Erasing " << s << '\n';
            break;
        }
}

chatroom_dict::chatroom_dict() {
    this->data[""] = vector<string>();
}

void chatroom_dict::reg(string name, int sfd) {
    lock_guard<mutex> lock(mtx);
    this->names[name] = sfd;
    this->data[""].push_back(name);
    //    for (auto i = this->names.begin(); i != this->names.end(); i++)
    //        cout << i->first << '\t' << i->second << '\n';
}

int chatroom_dict::push(string key, string client, bool create) {
    lock_guard<mutex> lock(mtx);
    string s = client;
    try { 
        this->data.at(key).push_back(client);        
        return 0;
    }
    catch (const out_of_range& e) {
        cerr << "Pushing " << client << " into new room " << key << '\n';
        if (create) {
            vector<string> v;
            v.push_back(client);
            this->data[key] = v;
            return 0;
        }
        else return 1;
    }
}

int chatroom_dict::remove(string key, string client) {
    lock_guard<mutex> lock(mtx);
    string s = client;

    try { 
        del(this->data.at(key), s); 
        if (this->data.at(key).empty())
            this->data.erase(key);
        return 0;
    }
    catch (const out_of_range& e) { return 1; }
}

const vector<string>& chatroom_dict::at(string key) {
    lock_guard<mutex> lock(mtx);
    return this->data.at(key);
}

vector<int> chatroom_dict::cons_at(string key) {
    //lock_guard<mutex> lock(mtx);
    vector<int> ret;

    for (auto name : this->at(key))
        ret.push_back(this->names.at(name));
    return ret;
}

vector<string> chatroom_dict::keys() {
    lock_guard<mutex> lock(mtx);
    vector<string> ret;

    for (auto it = this->data.begin(); it != this->data.end(); it++) {
        ret.push_back(it->first);
    }

    return ret;
}

pair<command, string> repl::read() {
    auto r = split(receive(this->con));

    try {
        if (r.at(0).compare("create") == 0) {
            if (r.at(1).compare("chatroom") == 0) 
                return pair<command, string>(CREATE, r.at(2));
            else return pair<command, string>(NO_OP, "");
        }

        else if (r.at(0).compare("list") == 0) {
            if (r.at(1).compare("chatrooms") == 0)
                return pair<command, string>(LIST_CHATS, "");
            else if (r.at(1).compare("users") == 0)
                return pair<command, string>(LIST_USERS, "");
            else return pair<command, string>(NO_OP, "");
        }

        else if (r.at(0).compare("join") == 0) 
            return pair<command, string>(JOIN, r.at(1));         

        else if (r.at(0).compare("leave") == 0)
            return pair<command, string>(LEAVE, "");

        else if (r.at(0).compare("add") == 0)
            return pair<command, string>(ADD, r.at(1));

        else if (r.at(0).compare("reply") == 0) {
            if (r.at(1).at(0) == '"') {
                string arg = "";
                for (auto i = 1; i < (int)r.size(); i++)
                    arg.append(r.at(i));
                cerr << '\n' << arg;
                return pair<command, string>(REPLY_M, arg.substr(1, arg.length() - 2));
            }
            else if (r.at(2).compare("tcp") == 0)
                return pair<command, string>(REPLY_TCP, r.at(1));
            else if (r.at(2).compare("udp") == 0)
                return pair<command, string>(REPLY_UDP, r.at(1));
            else return pair<command, string>(NO_OP, "");
        }

        else return pair<command, string>(NO_OP, "");
    } 
    catch (const out_of_range& e) {
        return pair<command, string>(NO_OP, "");
    }
}

void printv(vector<string> v) {
    for (auto it = v.begin(); it != v.end(); it++)
        cout << *it << '\n';
}

string repl::eval(command c, string arg) {
    //printv(chatrooms->at(this->room));

    switch (c) {
        case NO_OP:
            return "";

        case CREATE:
            if (this->room.compare("") == 0) {
                cerr << "HERE";
                this->chatrooms->push(arg, this->name, true);
                this->room = arg;
                this->chatrooms->remove("", this->name);
                return "Room created and joined.";
            }
            else return "Already in a room!";

        case LIST_CHATS:
            return join(this->chatrooms->keys());

        case JOIN:
            if (this->room.compare("") == 0) {
                if (this->chatrooms->push(arg, this->name, false) == 0) {
                    this->room = arg;
                    this->chatrooms->remove("", this->name);
                    return "Room joined.";
                }
                else return "Room does not exist!";
            }
            else return "Already in a room!";

        case LEAVE:
            if (this->room.compare("") == 0) 
                return "Not in any room!";
            else {
                this->chatrooms->remove(this->room, this->name);
                this->room = "";
                return "Left room.";
            }

        case LIST_USERS:
            if (this->room.compare("") == 0)
                return "Not in any room!";
            else 
                return join(this->chatrooms->at(this->room));

        case ADD:
            if (exists(this->chatrooms->at(""), arg) 
                    && this->room.compare("") != 0) {
                this->print(
                        string("*join ").append(this->room).append("|").append("You were added to a room.").c_str(), this->chatrooms->names.at(arg));
                return "Added user.";
            }
            else return "Could not add user!";

        case REPLY_M:
            if (this->room.compare("") != 0) {
                cerr << "REPLDHFDISHFSKLJFJKSD" << "\t" << this->room;
                this->broadcast(this->chatrooms->cons_at(this->room), this->name.append(": ").append(arg));
                cerr << "BROADCASTED\n";
                return "";
            }
            else return "You're not in a room!";


        case REPLY_TCP:
            if (this->room.compare("") != 0) {
                this->print(string("@").append(arg));
                string fcontent = this->receive(this->con);
                this->broadcast(this->chatrooms->cons_at(this->room), string("+").append(fcontent).append("|").append(arg));
            }

        case REPLY_UDP:
            if (this->room.compare("") != 0) {
                sockaddr client_addr;
                this->print(string("#").append(arg));
                char buffer[1000000], head[1000000];
                int q = (int)sizeof client_addr;
                if (recvfrom(this->con, buffer, 999999, 0, (sockaddr *)&client_addr, (unsigned int*)&q) == -1) {
                    perror("recvfrom");
                    exit(1);
                }
                this->broadcast(this->chatrooms->cons_at(this->room), "$");
                if (recvfrom(this->con, head, 999999, 0, (sockaddr *)&client_addr, (unsigned int*)&q) == -1) {
                    perror("recvfrom");
                    exit(1);
                }
                this->broadcast(this->chatrooms->cons_at(this->room), string("+").append(buffer).append("|").append(arg));

            }

        default:
            return "";
    }
}

int repl::print(string msg) { 
    return print(msg, con); 
}

string repl::receive(int sock) {
    char buf[1000000];

    if (recv(sock, buf, 999999, 0) == -1) {
        perror("recv");
        exit(1);
    }
    return string(buf);
}

vector<string> repl::split(string str) {
    vector<string> ss;
    char *s = new char[str.length() + 1];
    strcpy(s, str.c_str());

    for (auto token = strtok(s, " "); token != NULL; token = strtok(NULL, " "))
        ss.push_back(string(token));
    return ss;
}

string repl::join(const vector<string>& ss, string delimiter) {
    string s = "";
    for (auto it = ss.begin(); it != ss.end(); it++) {
        s.append(*it).append(delimiter);
    }
    return s;
}

int repl::print(string msg, int con) {
    cerr << msg << '\n';
    if (send(con, msg.c_str(), msg.length() + 1, 0) == 0) {
        perror("send");
        return 1;
    } else return 0;
}

void repl::broadcast(const vector<int>& clients, string msg) {
    cerr << "\nBROADCAST " << msg;

    cerr << '\n' << clients.empty() << '\n';

    for (auto user : clients) 
        print(msg, user);
} 
