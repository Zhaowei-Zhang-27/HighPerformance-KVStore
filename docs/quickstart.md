# 快速开始（Quick Start）

本页介绍如何编译、运行并测试 HighPerformance-KVStore。

---

## 📦 1. 环境要求

- Linux 或 WSL2（推荐 Ubuntu）
- g++（支持 C++11）
- cmake

检查命令：

```bash
g++ --version
cmake --version
⚙️ 2. 编译项目

进入项目根目录，然后执行：

cmake .
make


成功后会生成可执行文件：

./kv_store

🚀 3. 启动服务器
./kv_store
# 显示：
# Server started on port 8080...


默认监听：127.0.0.1:8080

🧪 4. 使用 nc 测试

打开一个终端：

nc 127.0.0.1 8080


然后输入：

SET name tesla
OK
GET name
tesla

🔥 5. 压力测试（benchmark）

如果仓库中包含 benchmark.cpp，可这样运行：

g++ benchmark.cpp -o benchmark -pthread
./benchmark


示例输出：

成功请求数: 100000
总耗时: 2.842 秒
QPS: 35186.5


通过以上步骤，你可以快速体验该 KV 存储服务器。