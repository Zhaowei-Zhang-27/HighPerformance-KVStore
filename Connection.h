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
    //补充：这里不需要判断了，因为已经拷贝的不包含换行符
    //if (!request.empty() && request.back() == '\n') request.pop_back();
    if (!request.empty() && request.back() == '\r') request.pop_back();

    // 2. 【切割】使用 stringstream 切分空格
    // 就像切香肠一样，把长字符串切成几个单词
    stringstream ss(request);
    string cmd, key, value;

    // "SET name zhangsan"  -> cmd="SET", key="name", value="zhangsan"
    // "GET name"           -> cmd="GET", key="name", value="" (空的)
    ss >> cmd >> key >> value;

    // 3. 【执行】根据指令干活
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
            
            // 1. 找到第一个换行符的位置
            size_t pos = readBuffer_.find('\n');
            
            // 2. 【切肉】把这一行切出来 (从0开始，切 pos 个字符)
            string command = readBuffer_.substr(0, pos);
            
            // 3. 【清盘】把切走的那一行，从 buffer 里删掉 (+1 是把 \n 也删了)
            // 这样 buffer 里剩下的就是下一条指令（或者是半截话）
            readBuffer_ = readBuffer_.substr(pos + 1);
            
            // 4. 【干活】解析 command 并执行 (原来的 process_command 逻辑放这里)
            // 为了代码干净，我们可以封装一个私有函数 parseAndExecute(command)
            // 或者直接在这里写解析逻辑...
            string response = process_command(command);
            
            // 这里为了演示思路，我先写个简单的回声逻辑，你等会填入真正的 KV 逻辑
            send(fd_, response.c_str(), response.size(), 0);
        }
    }

};

#endif