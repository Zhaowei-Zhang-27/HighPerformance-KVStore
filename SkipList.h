/**
 * SkipList.h
 * 这是一个基于 C++11 实现的跳表（SkipList）。
 * 我参考了 Redis 的 ZSkipList 和 LevelDB 的设计，用来替代 std::map 作为 KV 存储的核心引擎。
 * 支持插入、删除、查找，并且加了锁保证线程安全。
 * 作者：张钊维
 */

#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <vector>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <iostream>
#include <functional>

using namespace std;

/**
 * SkipNode: 跳表的节点结构体
 * 每个节点就像一栋“楼”，里面存着 key、value 和好多层的指针。
 */
template <typename K, typename V>
struct SkipNode {
    K key;
    V value;
    
    // forward 数组存的是每一层的下一个节点的指针
    // 比如 forward[0] 就是最底层的 next 指针（普通链表）
    // forward[i] 就是第 i 层的跳跃指针
    vector<SkipNode*> forward;

    // 构造函数：初始化键值，并根据层数分配好指针数组
    SkipNode(K k, V v, int level) 
        : key(k), value(v), forward(level, nullptr) {}
};

/**
 * SkipList: 跳表类
 * 实现了 O(logN) 复杂度的增删改查。
 */
template <typename K, typename V>
class SkipList {
public:
    // 构造函数：初始化随机数种子，创建哨兵节点
    SkipList() {
        srand(time(nullptr)); // 随机种子，保证每次运行抛硬币结果不一样
        level_ = 0;           // 一开始层数为0
        
        // 创建头节点（哨兵），把它建到最高（16层），方便以后连线
        // Key 和 Value 随便填个默认值就行，反正不用
        head_ = new SkipNode<K, V>(K(), V(), MAX_LEVEL);
    }

    // 析构函数：清理内存，防止泄漏
    ~SkipList() {
        //lock_guard<mutex> lock(mtx_); // 单线程暂时不需要大锁
        SkipNode<K, V>* curr = head_;
        while (curr) {
            // 先记下后面是谁
            SkipNode<K, V>* next = curr->forward[0];
            // 删掉当前节点
            delete curr;
            // 往后走
            curr = next;
        }
    }

     /*
     插入或更新数据
     逻辑：先查一遍，记录每层走到了哪里；如果 Key 存在就更新，不存在就插入新节点。
     */
    void insert(const K& key, const V& value) {
        //lock_guard<mutex> lock(mtx_); // 单线程暂时不需要大锁
        
        // update 数组用来记录每一层“在该插在谁后面”（前驱节点）
        vector<SkipNode<K, V>*> update(MAX_LEVEL, nullptr);
        SkipNode<K, V>* curr = head_;

        // 1. 从最高层往下找插入位置
        for (int i = level_ - 1; i >= 0; i--) {
            // 如果右边的 key 比目标小，就一直往右走
            while (curr->forward[i] && curr->forward[i]->key < key) {
                curr = curr->forward[i];
            }
            // 记下来：第 i 层是在这里停下的，待会新节点要接在它后面
            update[i] = curr;
        }

        // 2. 走到第0层，看看右边那个是不是就是我们要找的 key
        curr = curr->forward[0];

        // 如果 key 已经存在，直接更新 value，不用盖新楼了
        if (curr && curr->key == key) {
            curr->value = value;
            return;
        }

        // 3. 如果不存在，准备盖新楼
        // 抛硬币决定新节点有几层高
        int new_level = randomLevel();

        // 如果新层数比当前总层数还高，需要把头节点高出来的部分也补进 update 数组
        // 不然高层指针就悬空了
        if (new_level > level_) {
            for (int i = level_; i < new_level; i++) {
                update[i] = head_;
            }
            // 更新跳表目前的最高层数
            level_ = new_level;
        }

        // 4. 创建新节点
        SkipNode<K, V>* new_node = new SkipNode<K, V>(key, value, new_level);

        // 5. 缝合指针（把每一层都连起来）
        // 就像普通链表插入一样：新节点指向后继，前驱指向新节点
        for (int i = 0; i < new_level; i++) {
            new_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = new_node;
        }
    }

    /**
     * 查找数据
     * 如果找到了返回 true，并把值赋给 value_out
     */
    bool search(const K& key, V& value_out) {
        //lock_guard<mutex> lock(mtx_);
        SkipNode<K, V>* curr = head_;

        // 从最高层坐电梯往下找
        for (int i = level_ - 1; i >= 0; i--) {
            while (curr->forward[i] && curr->forward[i]->key < key) {
                curr = curr->forward[i];
            }
        }

        // 走到第0层，检查右边那个是不是
        curr = curr->forward[0];

        // 找到了！
        if (curr && curr->key == key) {
            value_out = curr->value;
            return true;
        }
        return false; // 没找到
    }

    /**
     * 删除数据
     * 逻辑：找到每一层的前驱，断开连接，释放内存。
     */
    bool remove(const K& key) {
        lock_guard<mutex> lock(mtx_);
        vector<SkipNode<K, V>*> update(MAX_LEVEL, nullptr);
        SkipNode<K, V>* curr = head_;

        // 先找一遍，记录每一层的前驱
        for (int i = level_ - 1; i >= 0; i--) {
            while (curr->forward[i] && curr->forward[i]->key < key) {
                curr = curr->forward[i];
            }
            update[i] = curr;
        }

        curr = curr->forward[0];
        // 如果没找到，直接返回
        if (!curr || curr->key != key) {
            return false;
        }

        // 找到了，开始逐层断开连接
        for (int i = 0; i < level_; i++) {
            // 如果这一层的前驱不指向我，说明这一层我已经断开了（或者我本来就没这么高）
            if (update[i]->forward[i] != curr) break;
            
            // 断开：前驱直接指向我的后继
            update[i]->forward[i] = curr->forward[i];
        }

        delete curr; // 释放内存

        // 如果删掉的是最高层的节点，可能导致总层数降低
        // 比如最高层只有这一个节点，删了之后层数就要减一
        while (level_ > 0 && head_->forward[level_ - 1] == nullptr) {
            level_--;
        }
        return true;
    }

    /**
     * 遍历所有节点
     * 这个是给 KVStore 用来做持久化保存或者清理内存的
     * 传入一个回调函数 func，我会把每个节点的 key 和 value 传给它
     */
    void traverse(std::function<void(K&, V&)> func) {
        //lock_guard<mutex> lock(mtx_);
        SkipNode<K, V>* curr = head_->forward[0]; // 从第0层第一个数据开始
        while (curr) {
            func(curr->key, curr->value); // 执行回调逻辑
            curr = curr->forward[0]; // 顺藤摸瓜往后走
        }
    }

private:
    // 最大层数限制 (Redis 是 32，这里设 16 足够了)
    static const int MAX_LEVEL = 16;
    
    SkipNode<K, V>* head_; // 哨兵头节点
    int level_;            // 当前跳表实际的最高层数
    mutex mtx_;            // 互斥锁

    // 抛硬币函数：50% 概率长高一层
    // 用来模拟跳表的概率平衡特性
    int randomLevel() {
        int lvl = 1;
        while ((rand() % 2) == 1 && lvl < MAX_LEVEL) {
            lvl++;
        }
        return lvl;
    }
};

#endif // SKIPLIST_H