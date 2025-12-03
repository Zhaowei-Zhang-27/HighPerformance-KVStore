# 通信协议（Protocol）

HighPerformance-KVStore 使用简单的文本协议，方便调试和学习。

---

## 📌 1. 文本命令格式

每条指令以换行符 `\n` 结尾。

当前支持两种命令：

---

## 🟦 SET 命令

SET <key> <value>\n

复制代码

示例：

SET name tesla

复制代码

服务器返回：

OK

yaml
复制代码

---

## 🟩 GET 命令

GET <key>\n

复制代码

示例：

GET name

vbnet
复制代码

如果 key 存在：

tesla

vbnet
复制代码

如果 key 不存在（视实现而定）：

NOT_FOUND

yaml
复制代码

---

## ⚠️ 2. 粘包与拆包

由于 TCP 是流式协议，有两种情况可能发生：

- 一次 read 读到多条指令（粘在一起）
- 一次 read 不足一条指令（半包）

为此，服务器为每个连接维护：

- **读缓冲区**
- **协议解析状态机**

解析方式：

1. 每次接收数据追加到读缓冲区  
2. 在缓冲区里找 `\n`  
3. 找到就解析为完整指令  
4. 剩余内容保留，等待下一次数据补齐  

---

## 🔄 3. 状态机解析流程

解析一条指令流程：

1. 发现换行符（表示命令结束）
2. 切分为指令和参数
3. 根据指令类型执行：
   - SET → 调用 KVStore::Set
   - GET → 调用 KVStore::Get
4. 构造响应并写入写缓冲区

---

## 🔮 4. 未来扩展（可选）

将来可以扩展支持：

- RESP 二进制协议（类似 Redis）
- Pipeline 批量指令
- 更多命令：DEL、EXISTS、INCR、EXPIRE 等
- 多 Key 操作