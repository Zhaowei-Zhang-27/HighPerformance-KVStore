/**
 * LV.1 任务：Linux TCP 服务端 (Echo Server)
 * 文件名: server.cpp
 * 编译命令: g++ server.cpp -o server
 * 运行命令: ./server
 */

#include <iostream>
#include <cstring>      // 用于 memset (清空内存)
#include <sys/socket.h> // 【核心】Linux Socket 系统调用 (socket, bind, listen, accept)
#include <netinet/in.h> // 【核心】网络地址结构体 (sockaddr_in)
#include <unistd.h>     // 【核心】通用文件操作 (read, write, close)

// 定义端口号：建议使用 1024 以上的端口，避免权限问题
#define PORT 8080

int main() {
    // =========================================================================
    // 1. 创建 Socket (买手机)
    // =========================================================================
    // AF_INET: 使用 IPv4 地址
    // SOCK_STREAM: 使用 TCP 协议 (流式传输，可靠)
    // 0: 自动选择默认协议 (即 TCP)
    // 返回值 server_fd: 文件描述符 (File Descriptor)，是操作这个 Socket 的唯一凭证
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // 检查是否创建成功 (返回 -1 代表失败)
    if (server_fd == -1) {
        perror("socket creation failed"); // perror 会自动打印具体的错误原因
        return -1;
    }

    // =========================================================================
    // 2. 准备地址信息 (写名片)
    // =========================================================================
    struct sockaddr_in address;
    
    // 必须把结构体内存清零，防止有垃圾数据导致绑定失败
    memset(&address, 0, sizeof(address));
    
    address.sin_family = AF_INET;           // 协议族：IPv4
    address.sin_addr.s_addr = INADDR_ANY;   // IP地址：0.0.0.0 (绑定本机所有网卡)
    
    // 【重要】端口号必须转换成“网络字节序”(大端序)
    // htons = Host to Network Short
    // 如果不转，8080 发出去可能会变成其他数字
    address.sin_port = htons(PORT);

    // =========================================================================
    // 3. 绑定端口 (插电话卡)
    // =========================================================================
    // (struct sockaddr*)&address:
    // bind 函数是通用的，支持多种协议。
    // 所以必须把 IPv4 专用的 sockaddr_in 强制转换成通用的 sockaddr 指针。
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return -1;
    }

    // =========================================================================
    // 4. 开始监听 (设置铃声)
    // =========================================================================
    // 3: "Backlog"，表示连接请求队列的最大长度。
    // 如果同时有 4 个人连上来，前 3 个排队，第 4 个会被直接拒绝。
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        return -1;
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    // =========================================================================
    // 5. 接受连接 (阻塞等待)
    // =========================================================================
    int new_socket;
    int addrlen = sizeof(address);
    
    // 【关键】程序运行到这里会卡住(Block)！直到有客户端连上来。
    // accept 返回一个新的 socket (new_socket)，专门用来跟这个特定的客户端聊天。
    // 原来的 server_fd 继续监听新的连接。
    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    
    if (new_socket < 0) {
        perror("accept failed");
        return -1;
    }
    
    std::cout << "Connection established! (有人连上来了)" << std::endl;

    // =========================================================================
    // 6. 读写数据 (聊天)
    // =========================================================================
    char buffer[1024] = {0};
    
    // read: 从 new_socket 里读取数据到 buffer
    // 返回值: 读到的字节数。如果返回 0，表示对方断开了连接。
    int valread = read(new_socket, buffer, 1024);
    
    if (valread > 0) {
        std::cout << "Client says: " << buffer << std::endl;

        // send: 给客户端回一句话
        const char* hello = "Hello from Tesla Server";
        send(new_socket, hello, strlen(hello), 0);
        std::cout << "Reply sent." << std::endl;
    }

    // =========================================================================
    // 7. 关闭连接 (挂电话)
    // =========================================================================
    close(new_socket); // 挂断当前通话
    close(server_fd);  // 关闭服务器 (通常服务器会有死循环，不关这个)

    return 0;
}