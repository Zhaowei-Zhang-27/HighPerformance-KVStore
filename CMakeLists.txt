cmake_minimum_required(VERSION 3.0)
project(TinyRedis)

# 设置编译选项，开启c++11，-g是用来调试的
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -Wall")

# 头文件路径
include_directories(.)

# 生成服务器可执行文件
add_executable(kv_store kv_store.cpp)

# 生成压测工具
# add_executable(test test.cpp) # 之前的测试文件，先注释掉
add_executable(benchmark benchmark.cpp)

# benchmark用到了多线程，必须链接pthread库，不然报错
target_link_libraries(benchmark pthread)