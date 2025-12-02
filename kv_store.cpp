
#include <iostream>     // cout, endl
#include <cstring>      // memset
#include <csignal>      // signal, SIGINT
#include <map>          // fdmap
#include <unistd.h>     // close
#include <fcntl.h>      // fcntl, O_NONBLOCK
#include <sys/socket.h> // socket, bind, listen, accept
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons
#include <netinet/tcp.h>
// 自定义头文件
#include "Epoller.h"
#include "Connection.h"
#include "KVStore.h"

using namespace std;

#define PORT 8080
#define MAX_EVENTS 1000
bool stop_server = false;
// 信号处理函数
void handle_signal(int sig) {
    //cout << "\n[Server] Caught signal " << sig << ", shutting down..." << endl;
    stop_server = true; 
}

KVStore g_store("data.db");
map<int, Connection*> fdmap; 
const int TIMEOUT = 10; 

 // 需要这个头文件

void set_nodelay(int sock) {
    int opt = 1;
    // 禁用 Nagle 算法，有数据立刻发，不等待
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}
// 工具函数：把 Socket 设置为“非阻塞”
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
    // 1. 创建监听 Socket
    signal(SIGINT, handle_signal);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 端口复用
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ::bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 100); 

    cout << "Epoll Server started on port " << PORT << endl;

    //创建 Epoll 对象
    Epoller epoller;
     // 监控EPOLLIN
     epoller.AddFd(server_fd,EPOLLIN);

    // =====================================================================
    // 4. 事件循环 (Event Loop)
    // =====================================================================
    while (!stop_server) {
        // -1 表示永久阻塞，直到有事件发生
        // nfds: 返回有多少个 socket 有事发生了
        int nfds = epoller.Wait(500);

        // 遍历所有有事的 Socket
        for (int i = 0; i < nfds; ++i) {
            
            // 情况 A: 如果是 server_fd 有事，说明【有新连接】来了！
            if (epoller.GetEventFd(i) == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

                
                if (client_sock > 0) {
                    //cout << "New Client Connected: " << client_sock << endl;
                    
                    // 关键：把新来的 client_sock 设为非阻塞
                    set_nonblocking(client_sock);
                    set_nodelay(client_sock);

                    // 关键：把新来的 client_sock 也拉进 Epoll 群里监控
                    epoller.AddFd(client_sock,EPOLLIN);
                    Connection *conn = new Connection(client_sock);
                    fdmap[client_sock] = conn;
                }
            }
            // 情况 B: 如果是其他 fd 有事，说明【客户端发数据】来了！
            else if (epoller.GetEvents(i) & EPOLLIN) {
                int sockfd = epoller.GetEventFd(i);
                Connection *cur = fdmap[sockfd];
                ssize_t n = cur->Read();
                if(n>0)
                {
                    cur->Process();
                }
                else 
                {
                    // valread == 0 表示客户端断开了连接
                    // valread < 0 表示出错
                    //cout << "Client " << sockfd << " disconnected." << endl;
                    
                    // 1. 从 Epoll 群里踢出去
                    epoller.DelFd(sockfd);
                    // 2. 关闭 Socket
                    delete fdmap[sockfd];
                    fdmap.erase(sockfd);
                }
            }
        }
        time_t now = time(nullptr); // 获取当前时间
        for (auto it = fdmap.begin(); it != fdmap.end(); ) {
            Connection* conn = it->second;
            
            // 检查：(当前时间 - 最后活跃时间) 是否超过 10秒
            if (now - conn->GetLastActiveTime() > TIMEOUT) {
                //cout << "[Timeout] Kicking client: " <<  conn->GetFd() << endl;
                
                // 1. 踢出 Epoll (不再监控)
                epoller.DelFd(conn->GetFd());
                
                // 2. 销毁对象 (析构函数会自动 close socket)
                delete conn;
                
                // 3. 【关键】从 map 移除，并让迭代器指向下一个
                // erase 会返回下一个有效的迭代器，这样就不会崩
                it = fdmap.erase(it); 
            } else {
                // 没超时，手动指向下一个
                it++;
            }
        }
    }

    close(server_fd);
    return 0;
}