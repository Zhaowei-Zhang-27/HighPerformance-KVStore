/**
 * 专用压测工具
 * 编译命令: g++ benchmark.cpp -o benchmark -pthread -std=c++11
 */

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <csignal>

using namespace std;

// ================= 配置区域 =================
const string SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 8080;
const int THREAD_COUNT = 50;        // 并发线程数
const int REQUESTS_PER_THREAD = 10000; 
// ===========================================

atomic<int> success_count(0);

//改成redis的输入格式
string ToResp(const vector<string>& args) {
    string res = "*" + to_string(args.size()) + "\r\n";
    for (const auto& arg : args) {
        res += "$" + to_string(arg.size()) + "\r\n" + arg + "\r\n";
    }
    return res;
}

void client_thread_func(int id) {
    try {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return;

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP.c_str(), &serv_addr.sin_addr);

        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            close(sock);
            return;
        }

        char buffer[1024];
        string key = "key_" + to_string(id);
        string value = "value_" + to_string(id);

        // 提前生成好命令字符串，避免在循环里反复拼接浪费 CPU
        vector<string> set_args = {"SET", key, value};
        string set_cmd = ToResp(set_args);
        vector<string> get_args = {"GET", key};
        string get_cmd = ToResp(get_args);

        for (int i = 0; i < REQUESTS_PER_THREAD; ++i) {
            // 1. 发送 SET
            if (send(sock, set_cmd.c_str(), set_cmd.length(), 0) < 0) break;
            if (read(sock, buffer, 1024) <= 0) break; 

            // 2. 发送 GET
            if (send(sock, get_cmd.c_str(), get_cmd.length(), 0) < 0) break;
            if (read(sock, buffer, 1024) > 0) {
                success_count++;
            } else {
                break;
            }
        }
        close(sock);
    } catch (...) {}
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    cout << "准备开始压测" << endl;
    cout << "线程数: " << THREAD_COUNT << ", 每线程请求: " << REQUESTS_PER_THREAD << endl;

    vector<thread> threads;
    auto start_time = chrono::high_resolution_clock::now();

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back(client_thread_func, i);
    }

    for (auto& t : threads) {
        if(t.joinable()) t.join();
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
    
    double seconds = (duration == 0) ? 0.001 : duration / 1000.0;
    double qps = success_count / seconds;

    cout << "\n✅ 压测完成!" << endl;
    cout << "成功请求数: " << success_count << endl;
    cout << "总耗时: " << seconds << " 秒" << endl;
    cout << "QPS: " << qps << " (次/秒)" << endl;

    return 0;
}