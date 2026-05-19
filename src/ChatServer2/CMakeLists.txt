# 收集所有源文件
aux_source_directory(. SRC_LIST)

# 定义可执行文件
add_executable(ChatServer2 ${SRC_LIST})

# 包含目录
target_include_directories(ChatServer2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
    ${MYSQLCPPCONN_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(ChatServer2 PRIVATE
    ${Boost_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    gRPC::grpc++
    mysqlcppconn
    ${HIREDIS_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    pthread
)

# 设置编译选项
target_compile_options(ChatServer2 PRIVATE -Wall -Wextra)
