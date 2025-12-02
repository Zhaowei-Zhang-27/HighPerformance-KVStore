/**
 * 专用压测工具 (防崩溃版)
 * 编译命令: g++ benchmark.cpp -o benchmark -pthread
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
#include <csignal> // 【新】

using namespace std;

// ================= 配置区域 =================
const string SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 8080;
const int THREAD_COUNT = 50;        // 先用 1 个线程测通
const int REQUESTS_PER_THREAD = 2000; 
// ===========================================

atomic<int> success_count(0);

void client_thread_func(int id) {
    try {
        // 使用 cerr 打印，cerr 不带缓冲，哪怕崩了也能显示出来
        cerr << "[Thread " << id << "] 启动..." << endl;

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("Socket creation failed");
            return;
        }

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP.c_str(), &serv_addr.sin_addr);

        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            cerr << "[Thread " << id << "] 连接服务器失败! (请检查 kv_store 是否运行)" << endl;
            close(sock);
            return;
        }
        
        // cerr << "[Thread " << id << "] 连接成功!" << endl;

        char buffer[1024];
        string key = "key_" + to_string(id);
        string value = "value_" + to_string(id);

        for (int i = 0; i < REQUESTS_PER_THREAD; ++i) {
            // SET
            string set_cmd = "SET " + key + " " + value + "\n";
            if (send(sock, set_cmd.c_str(), set_cmd.length(), 0) < 0) break; // 发送失败退出
            if (read(sock, buffer, 1024) <= 0) break; // 读取失败退出

            // GET
            string get_cmd = "GET " + key + "\n";
            if (send(sock, get_cmd.c_str(), get_cmd.length(), 0) < 0) break;
            if (read(sock, buffer, 1024) > 0) {
                success_count++;
            } else {
                break;
            }
        }

        close(sock);
        // cerr << "[Thread " << id << "] 任务完成。" << endl;

    } catch (const std::exception& e) {
        cerr << "[Thread " << id << "] 异常: " << e.what() << endl;
    }
}

int main() {
    // 【关键】忽略 SIGPIPE，防止写入断开的连接时导致程序崩溃
    signal(SIGPIPE, SIG_IGN);

    cout << "🔥 准备开始..." << endl;

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
    cout << "⚡ QPS: " << qps << " (次/秒)" << endl;

    return 0;
}