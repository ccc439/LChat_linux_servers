cmake_minimum_required(VERSION 3.10)
project(project_servers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

# 查找依赖
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(PkgConfig REQUIRED)

# 使用 pkg-config 查找库
pkg_check_modules(MYSQLCPPCONN REQUIRED mysqlclient)
pkg_check_modules(HIREDIS REQUIRED hiredis)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 添加子目录
add_subdirectory(src/ChatServer)
add_subdirectory(src/ChatServer2)
add_subdirectory(src/GateServer)
add_subdirectory(src/StatusServer)
