# HighPerformance-KVStore 🚀

![Language](https://img.shields.io/badge/language-C%2B%2B11-blue)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20WSL2-lightgrey)
![Build](https://img.shields.io/badge/build-CMake-green)
![License](https://img.shields.io/badge/license-MIT-orange)

## 📖 项目简介 (Introduction)
这是一个基于 **C++11** 实现的高性能、单线程、非阻塞网络服务器。
核心架构采用 **Reactor 事件驱动模型**，基于 Linux **Epoll ET (边缘触发)** 机制实现。
项目旨在模仿 **Redis** 的核心网络模型，实现了一个轻量级的 **Key-Value 存储引擎**，能够处理高并发连接，并支持数据持久化。

**核心亮点：**
* **高性能**：单线程 Reactor 模型，在 WSL2 环境下压测 QPS 突破 **3.5w+**。
* **高并发**：使用 Epoll IO 多路复用 + 非阻塞 IO，理论支持万级并发连接。
* **健壮性**：实现应用层读写缓冲区（解决 TCP 粘包/拆包）、定时器剔除空闲连接。
* **持久化**：支持内存数据快照（Snapshot）保存至硬盘，重启自动加载 (Crash-safe)。
* **优雅退出**：捕获 SIGINT 信号，确保数据安全落盘后关闭服务器。

## 🏗️ 核心架构 (Architecture)

* **Epoller**: 封装 `epoll_create`, `epoll_ctl`, `epoll_wait`，提供 RAII 风格的安全接口。
* **Connection**: 管理每个 TCP 连接的生命周期，包含读缓冲区和自定义协议解析状态机。
* **KVStore**: 负责核心数据的内存存储（基于 `std::map`）与文件持久化 (Load/Save)。
* **Reactor**: 主循环负责事件分发，将 IO 事件与业务逻辑解耦，实现高效调度。

## 📊 性能测试 (Benchmark)

### 测试环境
* **系统**: Ubuntu 20.04 LTS (WSL2 on Windows)
* **网络**: Localhost Loopback
* **压测工具**: 自研多线程压测客户端 (`benchmark.cpp`)

### 测试结果
在 **50 并发线程**、**长连接**、**读写混合 (1:1)** 的场景下：
```bash
✅ 压测完成!
成功请求数: 100,000
总耗时: 2.842 秒
⚡ QPS: 35,186.5 (次/秒)