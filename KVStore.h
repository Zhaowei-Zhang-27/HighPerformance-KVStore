#ifndef KVSTORE_H
#define KVSTORE_H

#include <map>
#include <string>
#include <fstream>  // 文件流库 (file stream)
#include <iostream>
#include <mutex> 

using namespace std;

class KVStore {
public:
    // 构造函数：初始化 filename_，然后调用 LoadFromFile()
    explicit KVStore(const string& filename):filename_(filename){
        LoadFromFile();
    };
    // 析构函数：调用 SaveToFile()
    ~KVStore()
    {
        
        SaveToFile();
    }
    // Set 函数 (加锁)
    void Set(const string& key,const string& value)
    {
        lock_guard<mutex> lock(mtx_);
        data_[key]=value;
    }
    // Get 函数 (加锁)
    string Get(const string& key)
    {
        lock_guard<mutex> lock(mtx_);
        if(data_.count(key)) return data_[key];
        return "";
    }

private:
    // 成员变量：data_, filename_, mtx_
    map<string,string> data_;
    string filename_;
    mutex mtx_;

    void LoadFromFile()
    {
        lock_guard<mutex> lock(mtx_);
        ifstream infile(filename_);
        if(!infile.is_open())
        {
            return; 
        }
        string key,value;
        while(infile>>key>>value)
        {
            data_[key]=value;
        }
        infile.close();
        cout << "[KVStore] Loaded " << data_.size() << " records." << endl;
    }
     void SaveToFile()
    {
        lock_guard<mutex> lock(mtx_);
        ofstream outfile(filename_);
        if(!outfile.is_open())
        {
            return;
        }
        for(const auto& pair:data_)
        {
            outfile<<pair.first<<' '<<pair.second<<"\n";
        }
        outfile.close();
        cout << "[KVStore] Saved " << data_.size() << " records to disk." << endl;
    }
    
};

#endif