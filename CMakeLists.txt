cmake_minimum_required(VERSION 3.10)
project(KVStoreEngine)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找系统里的线程库 (因为 benchmark.cpp 用到了多线程)
find_package(Threads REQUIRED)

# ==========================================
# 目标 1: 你的服务器 (kv_store)
# ==========================================
# 只需要编译 kv_store.cpp (头文件它会自动找)
add_executable(kv_store kv_store.cpp)

# ==========================================
# 目标 2: 你的压测工具 (benchmark)
# ==========================================
# 只需要编译 benchmark.cpp
add_executable(benchmark benchmark.cpp)

# 【关键】给 benchmark 加上线程库链接 (-pthread)
# 否则编译 benchmark 时会报错 "undefined reference to pthread_create"
target_link_libraries(benchmark Threads::Threads)