#ifndef CONNECTION_H
#define CONNECTION_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <unistd.h> // read, write, close
#include "KVStore.h"
#include <ctime>

using namespace std;

extern KVStore g_store;

class Connection{


private:
    int fd_;
    string readBuffer_;
    time_t last_active_time_;
    string process_command(string request) {
    if (!request.empty() && request.back() == '\r') request.pop_back();
    
    stringstream ss(request);
    string cmd, key, value;
    ss >> cmd >> key >> value;

    if (cmd == "SET") {
        // 格式检查：必须有 Key 和 Value
        if (key.empty() || value.empty()) {
            return "ERROR: Usage: SET <key> <value>\n";
        }
        
        // 核心动作：存入全局 Map
        g_store.Set(key, value);
        return "OK\n";
    }
    else if (cmd == "GET") {
        // 格式检查：必须有 Key
        if (key.empty()) {
            return "ERROR: Usage: GET <key>\n";
        }

        // 核心动作：在 Map 里查找
        // count(key) 返回 1 表示存在，0 表示不存在
        string val = g_store.Get(key);
        if (val == "") 
        {
            return "NULL\n";
        } 
        else 
        {
            return val + "\n";
        }
    }
    else {
        return "ERROR: Unknown Command. Try SET or GET.\n";
    }
}

public:
    explicit Connection(int fd):fd_(fd){
        last_active_time_ = time(nullptr);
    };
    ~Connection()
    {
        if (fd_ != -1) {
            close(fd_);
            //cout << "Connection closed: " << fd_ << endl;
        }
    }
    int GetFd() const { return fd_; }
    ssize_t Read()
    {
        char buff[1024]={0};
        ssize_t n=read(fd_,buff,sizeof(buff));
        if(n>0)
        {
            readBuffer_+=buff;
            last_active_time_ = time(nullptr);
        }
        return n;
    }
    time_t GetLastActiveTime() const { return last_active_time_; }
    void Process() {
        // 循环检查：只要 buffer 里有换行符，就说明有一句完整指令
        while (readBuffer_.find('\n') != string::npos) {
            
            size_z pos = readBuffer_.find('\n');
            
            string command = readBuffer_.substr(0, pos);
            
            readBuffer_ = readBuffer_.substr(pos + 1);
        
            string response = process_command(command);
            
            send(fd_, response.c_str(), response.size(), 0);
        }
    }

};

#endif