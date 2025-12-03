# HighPerformance-KVStore

一个基于 **C++11** 实现的高性能、单线程、非阻塞网络 KV 存储服务器，核心采用 **Reactor 事件驱动模型 + Epoll ET**，参考 Redis 网络模型设计，支持高并发连接与数据持久化。

## 核心特性

- 🚀 **高性能**：单线程 Reactor 模型，在 WSL2 环境下压测 QPS 突破 **35k+**
- 📡 **高并发**：`epoll` + 非阻塞 IO，多路复用，理论支持万级并发连接
- 🧱 **健壮性**：
  - 应用层读写缓冲区，解决 TCP 粘包 / 拆包
  - 自定义协议 + 状态机解析
  - 定时器剔除长时间空闲连接
- 💾 **持久化**：
  - 内存数据快照（Snapshot）落盘
  - 服务重启自动加载，提供 Crash-safe 保障
- 🎯 **优雅退出**：
  - 捕获 `SIGINT`（Ctrl + C）
  - 先触发数据保存，再关闭监听和连接

## 仓库地址

- GitHub: https://github.com/Zhaowei-Zhang-27/HighPerformance-KVStore  
- Gitee（国内镜像）: https://gitee.com/Zhaowei-Zhang-27/HighPerformance-KVStore  
