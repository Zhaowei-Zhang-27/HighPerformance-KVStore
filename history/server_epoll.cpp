/**
 * LV.3 任务：Epoll 高并发服务器 (单线程版)
 * 文件名: server_epoll.cpp
 * 编译命令: g++ server_epoll.cpp -o server_epoll
 * (注意：Epoll 是 Linux 独有的，Windows 上没有这个头文件，必须在 WSL 里跑)
 */

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>      // 【新】设置非阻塞
#include <sys/epoll.h>  // 【新】Epoll 头文件

using namespace std;

#define PORT 8080
#define MAX_EVENTS 1000 // 一次最多处理 1000 个就绪事件

// =========================================================================
// 工具函数：把 Socket 设置为“非阻塞” (Non-blocking)
// =========================================================================
// 为什么要非阻塞？
// 因为 Epoll 告诉你“有数据”时，你如果不一次性读完，或者读的时候卡住了，
// 整个线程就卡住了。为了配合 Epoll，Socket 必须是非阻塞的。
void set_nonblocking(int sock) {
    int opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return;
    } 
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)");
        return;
    }
}

int main() {
    // 1. 创建监听 Socket (跟以前一样)
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 端口复用
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 100); // 积压队列可以设大点

    cout << "Epoll Server started on port " << PORT << endl;

    // =====================================================================
    // 2. 创建 Epoll 实例 (第一斧：建群)
    // =====================================================================
    int epfd = epoll_create(1); // 参数只要大于0就行
    
    // 准备一个结构体，用来描述我们要监控的事件
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS]; // 用来接收内核返回的就绪事件列表

    // =====================================================================
    // 3. 把 server_fd (门口迎宾) 加入 Epoll 监控 (第二斧：拉人)
    // =====================================================================
    ev.data.fd = server_fd;     // 监控谁？监控 server_fd
    ev.events = EPOLLIN;        // 监控什么事？EPOLLIN (有数据进来了 = 有人连上来)
    
    // EPOLL_CTL_ADD: 添加监控
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

    // =====================================================================
    // 4. 事件循环 (Event Loop)
    // =====================================================================
    while (true) {
        // (第三斧：等通知)
        // -1 表示永久阻塞，直到有事件发生
        // nfds: 返回有多少个 socket 有事发生了
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);

        // 遍历所有有事的 Socket
        for (int i = 0; i < nfds; ++i) {
            
            // 情况 A: 如果是 server_fd 有事，说明【有新连接】来了！
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_sock > 0) {
                    cout << "New Client Connected: " << client_sock << endl;
                    
                    // 关键：把新来的 client_sock 设为非阻塞
                    set_nonblocking(client_sock);

                    // 关键：把新来的 client_sock 也拉进 Epoll 群里监控
                    ev.data.fd = client_sock;
                    ev.events = EPOLLIN | EPOLLET; // 监听读事件 | 边缘触发模式(ET)
                    epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &ev);
                }
            }
            // 情况 B: 如果是其他 fd 有事，说明【客户端发数据】来了！
            else if (events[i].events & EPOLLIN) {
                int sockfd = events[i].data.fd;
                char buffer[1024] = {0};
                
                // 读取数据
                int valread = read(sockfd, buffer, 1024);
                
                if (valread > 0) {
                    // 读到了数据
                    cout << "[Client " << sockfd << "] says: " << buffer << endl;
                    
                    // 简单回复 HTTP 响应
                    string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello from Epoll!</h1>";
                    send(sockfd, response.c_str(), response.length(), 0);
                }
                else {
                    // valread == 0 表示客户端断开了连接
                    // valread < 0 表示出错
                    cout << "Client " << sockfd << " disconnected." << endl;
                    
                    // 1. 从 Epoll 群里踢出去
                    epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
                    // 2. 关闭 Socket
                    close(sockfd);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}