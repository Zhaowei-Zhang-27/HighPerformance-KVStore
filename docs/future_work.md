# 未来优化方向（Future Work）

HighPerformance-KVStore 当前是一个学习与实践性质的高性能 KV 存储项目，仍有大量可扩展空间。本页总结可继续提升的方向。

---

## 🔧 1. 多线程 / 多 Reactor

当前采用单线程 Reactor，可尝试：

- 主线程只负责 accept
- 多个 Reactor 或线程池处理读写事件
- 每个 Reactor 维护自己的 epoll 实例
- 使用负载均衡策略分配连接

这将充分利用多核 CPU，提高吞吐量。

---

## 🗂️ 2. KV 存储结构升级

目前使用 `std::map`，可优化为：

- `unordered_map`（O(1) 平均复杂度）
- Sharding 分片（在多线程下减少锁竞争）
- 增加更多数据类型（string/int/list/map）

---

## 🔌 3. 协议扩展

从简单文本协议扩展为更专业的协议：

- Redis RESP（二进制协议）
- 支持 Pipeline 批量命令
- 新增指令：
  - DEL
  - EXISTS
  - INCR
  - EXPIRE（带过期时间）
  - 清空数据库等

---

## 💾 4. 持久化增强

当前为快照（Snapshot），可继续提升：

- AOF（Append Only File）日志形式
- 后台线程异步持久化（避免阻塞主线程）
- 合并 AOF 日志，减少磁盘占用
- 增强崩溃恢复能力

---

## 📊 5. 监控与可视化

可新增：

- 当前 QPS
- 活跃连接数
- 内存占用
- 运行时统计（累计请求数、失败数）

并可选：

- 提供 HTTP 管理页面
- CLI 管理界面

---

## 🧪 6. 测试与工程化

- 增加单元测试（KVStore、协议解析、连接管理）
- 增加集成测试（端到端 GET/SET 测试）
- 引入日志库（spdlog 等）
- 加入 CI/CD 构建流程（自动编译 + 测试）
- 提供 Dockerfile / docker-compose

---

## 🧱 7. 更高层的扩展方向

- 键过期策略（类似 Redis 的惰性删除 + 定期删除）
- 数据持久化目录权限管理
- 多数据库实例（类似 Redis 的 db 号）
- 内存淘汰策略（LRU / LFU）