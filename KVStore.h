#ifndef KVSTORE_H
#define KVSTORE_H

#include "SkipList.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>


using namespace std;

enum ObjType {
    OBJ_STRING = 0,
    OBJ_LIST   = 1
};

struct RedisObject {
    ObjType type;
    void* ptr;
    RedisObject(ObjType t, void* p) : type(t), ptr(p) {}
    ~RedisObject() {
        if (type == OBJ_STRING) delete (string*)ptr;
        else if (type == OBJ_LIST) delete (vector<string>*)ptr;
    }
};

class KVStore {
public:
    explicit KVStore(const string& filename) : filename_(filename) {
        LoadFromFile();
    }

    ~KVStore() {
        SaveToFile();
        // 退出时，SkipList 析构会删节点，但我们需要先删节点里的 RedisObject
        auto free_func = [](string& key, RedisObject*& val) {
            delete val;
        };
        data_.traverse(free_func);
    }

    void Set(const string& key, const string& value) {
        // 1. 查旧删旧
        RedisObject* old_obj = nullptr;
        if (data_.search(key, old_obj)) {
            delete old_obj;
        }
        // 2. 立新
        string* str_ptr = new string(value);
        RedisObject* new_obj = new RedisObject(OBJ_STRING, str_ptr);
        data_.insert(key, new_obj);
    }

    string Get(const string& key) {
        RedisObject* obj = nullptr;
        if (data_.search(key, obj)) {
            if (obj->type == OBJ_STRING) {
                return *(string*)(obj->ptr);
            }
        }
        return "";
    }

    // 修改前：void LPush(...)
    // 修改后：int LPush(...)
    int LPush(const string& key, const string& value) {
        
        RedisObject* obj = nullptr;

        // 1. 查找
        if (data_.search(key, obj)) {
            // 找到了，但类型不对，返回错误码 -1
            if (obj->type != OBJ_LIST) return -1;
        } else {
            // 没找到 -> 新建 List
            vector<string>* vec = new vector<string>();
            obj = new RedisObject(OBJ_LIST, vec);
            data_.insert(key, obj);
        }

        // 2. 此时 obj 肯定是对的
        vector<string>* vec = (vector<string>*)(obj->ptr);
        vec->push_back(value);
        
        // 3. 返回列表当前的长度
        return vec->size();
    }
    string LRange(const string& key) {
        RedisObject* obj = nullptr;
        if (!data_.search(key, obj)) return "*0\r\n";
        if (obj->type != OBJ_LIST) return "-ERR WRONGTYPE\r\n";

        vector<string>* vec = (vector<string>*)(obj->ptr);
        string res = "*" + to_string(vec->size()) + "\r\n";
        for (const string& s : *vec) {
            res += "$" + to_string(s.size()) + "\r\n" + s + "\r\n";
        }
        return res;
    }

private:
    SkipList<string, RedisObject*> data_;
    string filename_;
    

    void SaveToFile() {
        ofstream outfile(filename_);
        if (!outfile.is_open()) return;
        int count = 0;
        auto save_func = [&](const string& key, RedisObject* val) {
            if (val->type == OBJ_STRING) {
                outfile << "0 " << key << " " << *(string*)val->ptr << "\n";
            } else if (val->type == OBJ_LIST) {

                vector<string>* vec = (vector<string>*)val->ptr;
                outfile << "1 " << key << " " << vec->size();
                for (const auto& s : *vec) outfile << " " << s;
                outfile << "\n";
            }
            count++;
        };
        data_.traverse(save_func);
        outfile.close();
        cout << "[KVStore] Saved " << count << " records to disk." << endl;
    }

    void LoadFromFile() {
        ifstream infile(filename_);
        if (!infile.is_open()) return;
        int count = 0;
        int type;
        string key;
        while (infile >> type >> key) {
            if (type == OBJ_STRING) {
                string val;
                infile >> val;
                Set(key, val); // 直接调用 Set ,因为没有锁冲突了
            } else if (type == OBJ_LIST) {

                int size;
                infile >> size;
                string val;
                // 先建个空的占位
                // 更好的办法是直接调 LPush，虽然效率低点(每次都search)但代码复用高
                for (int i = 0; i < size; ++i) {
                    infile >> val;
                    LPush(key, val);
                }
            }
            count++;
        }
        infile.close();
        cout << "[KVStore] Loaded " << count << " records from disk." << endl;
    }
};

#endif