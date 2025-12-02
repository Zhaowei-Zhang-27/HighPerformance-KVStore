/**
 * LV.2 任务：多线程 Web 服务器
 * 文件名: server_mt.cpp
 * 编译命令: g++ server_mt.cpp -o server_mt -pthread
 * (注意：必须加 -pthread 参数，否则编译会报错！)
 */

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>       // 【新东西】C++ 多线程库
#include <sstream>      // 【新东西】字符串流 (用于构建 HTTP 响应)

#define PORT 8080

// =========================================================================
// 子线程的工作函数：专门负责跟一个特定的客户端聊天
// =========================================================================
void handle_client(int client_socket) {
    char buffer[1024] = {0};
    
    // 1. 读取客户端发来的 HTTP 请求
    // 浏览器连接上来后，会噼里啪啦发一大堆 HTTP 协议头给我们
    int valread = read(client_socket, buffer, 1024);
    
    if (valread > 0) {
        // 打印出来看看浏览器发了啥 (实际开发中不用打印)
        std::cout << "[线程 " << std::this_thread::get_id() << "] 收到请求:\n" << buffer << std::endl;

        // 2. 准备 HTTP 响应 (这就是所谓的“硬通货”字符串处理)
        // 哪怕只是发个 Hello，也要按 HTTP 格式拼凑
        std::string response = "HTTP/1.1 200 OK\r\n"; // 状态行
        response += "Content-Type: text/html\r\n";    // 告诉浏览器我发的是网页
        response += "\r\n";                           // 空行 (Header 和 Body 的分界线)
        response += "<html><body><h1>Hello from C++ Multithreaded Server!</h1></body></html>"; // 内容

        // 3. 发送回去
        // c_str() 是把 C++ string 转成 C 语言的 char*，因为 send 只认 char*
        send(client_socket, response.c_str(), response.length(), 0);
        std::cout << "[线程 " << std::this_thread::get_id() << "] 响应已发送，任务结束。" << std::endl;
    }

    // 4. 关掉这个分机的连接 (如果不关，客户端会一直转圈圈)
    close(client_socket);
}

// =========================================================================
// 主线程：只负责在门口死等 (accept)
// =========================================================================
int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) return -1;

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 端口复用 (防止关掉程序后，端口几分钟内没法再用)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return -1;
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        return -1;
    }

    std::cout << "Multithreaded Server started on port " << PORT << "..." << std::endl;

    // 死循环：服务器永远不关机
    while (true) {
        int new_socket;
        int addrlen = sizeof(address);
        
        // 1. 主线程卡在这里等客人
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        
        if (new_socket >= 0) {
            std::cout << "Main Thread: New connection accepted! Spawning worker thread..." << std::endl;

            // 2. 【新东西】来客了？立刻创建一个子线程！
            // std::thread(函数名, 参数)
            // 意思是：启动一个新线程，去执行 handle_client(new_socket)
            std::thread t(handle_client, new_socket);

            // 3. 【新东西】分离线程
            // detach 意思是：你去忙你的吧，不用向我汇报，干完活你自己销毁。
            // 如果不写 detach，主线程在 t 对象销毁时会报错。
            t.detach();
        }
    }

    return 0;
}