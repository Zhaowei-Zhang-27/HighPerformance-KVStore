/* client.cpp - 发起连接的人 */
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

int main() {
    // 1. 买手机
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // 2. 查电话号
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // 把字符串 "127.0.0.1" (本机) 转成网络地址
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // 3. 拨号 (发起三次握手)
    std::cout << "正在连接服务端..." << std::endl;
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("连接失败");
        return -1;
    }
    std::cout << "连接成功！" << std::endl;

    // 4. 说话
    const char* msg = "Hello Tesla, I am Client!";
    send(sock, msg, strlen(msg), 0);
    std::cout << "消息已发送: " << msg << std::endl;

    // 5. 听对方回话
    char buffer[1024] = {0};
    read(sock, buffer, 1024);
    std::cout << "收到服务端回复: " << buffer << std::endl;

    // 6. 挂电话
    close(sock);
    return 0;
}