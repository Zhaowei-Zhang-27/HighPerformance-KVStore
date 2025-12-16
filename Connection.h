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
#include <algorithm>

using namespace std;

extern KVStore g_store;

class Connection{


private:
    int fd_;
    string readBuffer_;
    time_t last_active_time_;

    enum State {
        STATE_REQ_NUM,  // 状态 A: 读参数个数 (*3)
        STATE_ARG_LEN,  // 状态 B: 读参数长度 ($3)
        STATE_ARG_DATA  // 状态 C: 读参数数据 (SET)
    };

    State state_ = STATE_REQ_NUM; // 当前状态，默认是 A
    std::vector<string> args_; // 存解析出来的参数 (如 {"SET", "key", "val"})
    int expectedArgs_ = 0;     // 还要读几个参数？ (对应 *3)
    int expectedLen_ = 0;      // 当前参数的长度是多少？ (对应 $3)

    // 【新版】业务逻辑：处理解析好的参数列表，返回符合 RESP 格式的字符串
    string process_command(const vector<string>& args) {
        if (args.empty()) return "";

        string cmd = args[0];
        transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
        //处理 SET 命令
        if (cmd == "SET") {
            if (args.size() < 3) {
                return "-ERR wrong number of arguments for 'set' command\r\n";
            }
            string key = args[1];
            string value = args[2];
            
            g_store.Set(key, value);
            return "+OK\r\n"; // 告诉客户端：成功了
        }
        
        //处理 GET 命令
        else if (cmd == "GET") {
            if (args.size() < 2) {
                return "-ERR wrong number of arguments for 'get' command\r\n";
            }
            string key = args[1];
            string val = g_store.Get(key);
            
            if (val == "") {
                return "$-1\r\n"; // Redis 的 nil (没找到)
            } else {
                // 拼凑 Bulk String 格式: $长度\r\n内容\r\n
                return "$" + to_string(val.size()) + "\r\n" + val + "\r\n";
            }
        }

        //处理 PING 命令
        else if (cmd == "PING") {
            return "+PONG\r\n";
        }
        //LPUSH 
        // ... 在 process_command 里 ...
        else if (cmd == "LPUSH") {
            if (args.size() < 3) return "-ERR wrong number of arguments for 'lpush' command\r\n";
            
            // 1. 获取返回值
            int len = g_store.LPush(args[1], args[2]);
            
            // 2. 判断结果
            if (len == -1) {
                // 类型不对，报 Redis 标准错误
                return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
            } else {
                // 成功，返回列表长度
                return ":" + to_string(len) + "\r\n";
            }
        }

        //LRANGE
        else if (cmd == "LRANGE") {
            if (args.size() < 4) return "-ERR wrong number of arguments for 'lrange' command\r\n";
            // args[1] 是 key，暂时忽略 args[2]和[3] (start/stop)
            return g_store.LRange(args[1]);
        }
        //未知命令
        else {
            return "-ERR unknown command '" + cmd + "'\r\n";
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
        // 循环检查：只要 buffer 里有\r\n，就说明有一句完整指令
            while (true) {
        // ===================================================
        // 状态 A: 读取参数个数 (*3)
        // ===================================================
        if (state_ == STATE_REQ_NUM) {
            size_t pos = readBuffer_.find("\r\n");
            if (pos == string::npos) break; // 没读完一行，等下次

            string line = readBuffer_.substr(0, pos);
            readBuffer_ = readBuffer_.substr(pos + 2);

            if (line.empty() || line[0] != '*') continue; // 容错

            expectedArgs_ = stoi(line.substr(1));
            args_.clear(); 
            
            state_ = STATE_ARG_LEN; // 去读下一行的长度
        }
        
        // ===================================================
        // 状态 B: 读取参数长度 ($3)
        // ===================================================
        else if (state_ == STATE_ARG_LEN) {
            size_t pos = readBuffer_.find("\r\n");
            if (pos == string::npos) break; // 没读完一行，等下次

            string line = readBuffer_.substr(0, pos);
            readBuffer_ = readBuffer_.substr(pos + 2);

            if (line.empty() || line[0] != '$') continue; // 容错

            expectedLen_ = stoi(line.substr(1));
            
            state_ = STATE_ARG_DATA; // 去读具体数据
        }
        
        // ===================================================
        // 状态 C: 读取参数内容 (SET / key / value)
        // ===================================================
        else if (state_ == STATE_ARG_DATA) {
            // +2 是因为数据后面还有 \r\n
            if (readBuffer_.size() < expectedLen_ + 2) {
                break; 
            }

            // 截取数据
            string data = readBuffer_.substr(0, expectedLen_);
            readBuffer_ = readBuffer_.substr(expectedLen_ + 2); // 删掉数据和\r\n

            args_.push_back(data);
            expectedArgs_--;

                if (expectedArgs_ == 0) 
                    {
                // 凑齐了！执行命令
                string response = process_command(args_); // 你的业务函数
                send(fd_, response.c_str(), response.size(), 0);
                
                state_ = STATE_REQ_NUM; // 重置回状态 A
                    } 
                else 
                    {
                // 还没齐，继续去读下一个参数的长度
                state_ = STATE_ARG_LEN; 
                    }
                }
            }
        }
    

};

#endif