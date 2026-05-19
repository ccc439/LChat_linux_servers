# Project Servers - 分布式聊天服务器系统

## 📋 项目概述

这是一个基于 C++ 和 Node.js 的分布式聊天服务器系统，包含多个微服务组件。本项目采用 gRPC 进行服务间通信，使用 MySQL 存储数据，Redis 作为缓存。

---

## 🛠️ 环境配置指南

### 1. 系统要求

- **操作系统**: Linux (推荐 Ubuntu 20.04+)
- **编译器**: GCC 7+ (支持 C++17)
- **CMake**: 3.10+
- **Node.js**: 18.x+ (用于 VerifyServer)

### 2. 安装依赖

#### 2.1 基础开发工具

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git pkg-config
```

#### 2.2 C++ 库依赖

```bash
# Boost 库
sudo apt-get install -y libboost-all-dev

# Protocol Buffer 和 gRPC
sudo apt-get install -y libprotobuf-dev protobuf-compiler grpc++ libgrpc++-dev

# MySQL Connector/C++
sudo apt-get install -y libmysqlcppconn-dev

# Redis 客户端库
sudo apt-get install -y libhiredis-dev

# JSON 库
sudo apt-get install -y libjsoncpp-dev

# OpenSSL
sudo apt-get install -y libssl-dev
```

#### 2.3 Node.js 依赖（VerifyServer）

```bash
# 安装 Node.js 18.x
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# 验证安装
node --version
npm --version
```

#### 2.4 运行时服务

```bash
# MySQL 服务器
sudo apt-get install -y mysql-server
sudo systemctl start mysql
sudo systemctl enable mysql

# Redis 服务器
sudo apt-get install -y redis-server
sudo systemctl start redis
sudo systemctl enable redis
```

---

## ⚙️ 配置文件设置

### ⚠️ 重要安全提示

**所有配置文件（`config.ini`）都包含敏感信息（数据库密码、IP地址等），已添加到 `.gitignore`，不会被提交到 Git。**

每个服务都需要独立的配置文件。项目提供了配置模板文件（`*.template`），您需要根据实际环境复制并修改。

### 3. 配置文件结构

```
project_servers/
├── config/
│   ├── GateServer/
│   │   └── config.ini.template    # 网关服务器配置模板
│   ├── ChatServer/
│   │   └── config.ini.template    # 聊天服务器1配置模板
│   ├── ChatServer2/
│   │   └── config.ini.template    # 聊天服务器2配置模板
│   └── StatusServer/
│       └── config.ini.template    # 状态服务器配置模板
```

### 4. 配置步骤

#### 4.1 GateServer 配置

```bash
cd project_servers/config/GateServer
cp config.ini.template config.ini
vim config.ini
```

**需要修改的配置项：**

```ini
[GateServer]
Port = 8080                    # 网关监听端口（客户端连接端口）

[VerifyServer]
Host = 127.0.0.1              # 验证码服务地址
Port = 50051                  # 验证码服务端口

[Mysql]
Host = YOUR_MYSQL_HOST        # 替换为实际MySQL主机IP
Port = YOUR_MYSQL_PORT        # 替换为实际MySQL端口
User = YOUR_MYSQL_USER        # 替换为MySQL用户名
Passwd = YOUR_MYSQL_PASSWORD  # 替换为MySQL密码
Schema = YOUR_DATABASE_NAME   # 替换为数据库名称

[Redis]
Host = YOUR_REDIS_HOST        # 替换为实际Redis主机IP
Port = YOUR_REDIS_PORT        # 替换为实际Redis端口
Passwd = YOUR_REDIS_PASSWORD  # 替换为Redis密码

[StatusServer]
Host = 127.0.0.1              # 状态服务器地址
Port = 50052                  # 状态服务器端口
```

#### 4.2 ChatServer 配置

```bash
cd project_servers/config/ChatServer
cp config.ini.template config.ini
vim config.ini
```

**需要修改的配置项：**

```ini
[SelfServer]
Name = chatserver1            # 服务器名称（唯一标识）
Host = 0.0.0.0                # 监听地址
Port = 8090                   # 业务端口
RPCPort = 50055               # RPC通信端口

[Mysql]
Host = YOUR_MYSQL_HOST        # MySQL主机
Port = YOUR_MYSQL_PORT        # MySQL端口
User = YOUR_MYSQL_USER        # MySQL用户
Passwd = YOUR_MYSQL_PASSWORD  # MySQL密码
Schema = YOUR_DATABASE_NAME   # 数据库名

[Redis]
Host = YOUR_REDIS_HOST        # Redis主机
Port = YOUR_REDIS_PORT        # Redis端口
Passwd = YOUR_REDIS_PASSWORD  # Redis密码

[PeerServer]
Servers = chatserver2         # 对等服务器列表（多服务器用逗号分隔）

[chatserver2]
Name = chatserver2            # 对等服务器名称
Host = 127.0.0.1             # 对等服务器地址（跨机器部署时改为实际IP）
Port = 50056                 # 对等服务器RPC端口
```

#### 4.3 ChatServer2 配置

```bash
cd project_servers/config/ChatServer2
cp config.ini.template config.ini
vim config.ini
```

**主要区别：**
- `Name = chatserver2`
- `Port = 8091`
- `RPCPort = 50056`
- 对等服务器指向 `chatserver1`

#### 4.4 StatusServer 配置

```bash
cd project_servers/config/StatusServer
cp config.ini.template config.ini
vim config.ini
```

**需要修改的配置项：**

```ini
[StatusServer]
Host = 127.0.0.1
Port = 50052

[chatservers]
Name = chatserver1,chatserver2  # 管理的聊天服务器列表

[chatserver1]
Name = chatserver1
Host = 127.0.0.1               # 跨机器部署时改为实际IP
Port = 8090

[chatserver2]
Name = chatserver2
Host = 127.0.0.1               # 跨机器部署时改为实际IP
Port = 8091
```

#### 4.5 VerifyServer 配置（Node.js）

VerifyServer 使用 JSON 配置文件，位于 `src/VerifyServer/`：

```bash
cd project_servers/src/VerifyServer
cp config.json.template config.json
vim config.json
```

**需要修改的配置项：**

```json
{
    "email": {
      "user": "YOUR_EMAIL_ADDRESS",        // 替换为邮箱地址（如：xxx@163.com）
      "pass": "YOUR_EMAIL_AUTH_CODE"       // 替换为邮箱授权码（非登录密码）
    },
    "mysql": {
        "host": "YOUR_MYSQL_HOST",         // MySQL主机地址
        "port": YOUR_MYSQL_PORT,           // MySQL端口
        "passwd": "YOUR_MYSQL_PASSWORD"    // MySQL密码
    },
    "redis": {
        "host": "YOUR_REDIS_HOST",         // Redis主机地址
        "port": YOUR_REDIS_PORT,           // Redis端口
        "passwd": "YOUR_REDIS_PASSWORD"    // Redis密码
    }
}
```

**重要提示：**
- 邮箱 `pass` 字段应填写**授权码**而非登录密码
- 以 163 邮箱为例，需要在邮箱设置中开启 SMTP 服务并获取授权码
- MySQL 和 Redis 配置应与其它服务保持一致

---

## 🔨 编译项目

### 5. 生成 Protocol Buffer 代码

在编译前，必须先生成 gRPC 代码：

```bash
cd project_servers

# 为每个服务生成代码
protoc -I=src/ChatServer \
  --cpp_out=src/ChatServer \
  --grpc_out=src/ChatServer \
  --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
  src/ChatServer/message.proto

protoc -I=src/ChatServer2 \
  --cpp_out=src/ChatServer2 \
  --grpc_out=src/ChatServer2 \
  --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
  src/ChatServer2/message.proto

protoc -I=src/GateServer \
  --cpp_out=src/GateServer \
  --grpc_out=src/GateServer \
  --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
  src/GateServer/message.proto

protoc -I=src/StatusServer \
  --cpp_out=src/StatusServer \
  --grpc_out=src/StatusServer \
  --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
  src/StatusServer/message.proto
```

### 6. 使用构建脚本（推荐）

```bash
cd project_servers

# Debug 模式
./scripts/build.sh Debug

# Release 模式
./scripts/build.sh Release
```

### 7. 手动编译

```bash
cd project_servers
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

编译后的可执行文件位于 `bin/` 目录。

---

## 🚀 部署与启动

### 8. 部署结构

每个服务应有独立的运行目录：

```
deploy/
├── GateServer/
│   ├── GateServer          # 可执行文件
│   └── config.ini          # 配置文件
├── ChatServer/
│   ├── ChatServer
│   └── config.ini
├── ChatServer2/
│   ├── ChatServer2
│   └── config.ini
├── StatusServer/
│   ├── StatusServer
│   └── config.ini
└── VerifyServer/
    ├── server.js
    ├── config.js
    └── node_modules/
```

### 9. 复制可执行文件和配置

```bash
# 创建部署目录
mkdir -p deploy/{GateServer,ChatServer,ChatServer2,StatusServer,VerifyServer}

# 复制可执行文件
cp bin/GateServer deploy/GateServer/
cp bin/ChatServer deploy/ChatServer/
cp bin/ChatServer2 deploy/ChatServer2/
cp bin/StatusServer deploy/StatusServer/

# 复制配置文件
cp config/GateServer/config.ini deploy/GateServer/
cp config/ChatServer/config.ini deploy/ChatServer/
cp config/ChatServer2/config.ini deploy/ChatServer2/
cp config/StatusServer/config.ini deploy/StatusServer/

# 复制 VerifyServer（注意：需要先配置config.json）
mkdir -p deploy/VerifyServer
cp src/VerifyServer/server.js deploy/VerifyServer/
cp src/VerifyServer/config.js deploy/VerifyServer/
cp src/VerifyServer/proto.js deploy/VerifyServer/
cp src/VerifyServer/email.js deploy/VerifyServer/
cp src/VerifyServer/redis.js deploy/VerifyServer/
cp src/VerifyServer/const.js deploy/VerifyServer/
cp src/VerifyServer/message.proto deploy/VerifyServer/
cp src/VerifyServer/package.json deploy/VerifyServer/

# ⚠️ 重要：在部署前，先在 deploy/VerifyServer/ 中创建 config.json
# cd deploy/VerifyServer
# cp ../../src/VerifyServer/config.json.template config.json
# vim config.json  # 修改为实际配置

cd deploy/VerifyServer && npm install
```

### 10. 启动服务

**启动顺序非常重要：** StatusServer → GateServer → ChatServer

```bash
cd deploy

# 1. 启动 StatusServer
cd StatusServer
nohup ./StatusServer > status.log 2>&1 &
sleep 3

# 2. 启动 GateServer
cd ../GateServer
nohup ./GateServer > gate.log 2>&1 &
sleep 3

# 3. 启动 ChatServer
cd ../ChatServer
nohup ./ChatServer > chat1.log 2>&1 &
sleep 3

# 4. 启动 ChatServer2（如果需要）
cd ../ChatServer2
nohup ./ChatServer2 > chat2.log 2>&1 &
sleep 3

# 5. 启动 VerifyServer
cd ../VerifyServer
nohup npm run serve > verify.log 2>&1 &
```

### 11. 使用启动脚本

项目提供了便捷的启动脚本：

```bash
cd project_servers/scripts

# 启动各个服务
./start_status.sh
./start_gate.sh
./start_chat.sh
./start_chat2.sh
```

---

## 🔥 防火墙配置

### 12. 开放必要端口

**对外服务端口（必须开放）：**
- `8080` - GateServer（客户端接入）
- `8090` - ChatServer1
- `8091` - ChatServer2

**内部RPC端口（不对外开放）：**
- `50051` - VerifyServer
- `50052` - StatusServer
- `50055` - ChatServer1 RPC
- `50056` - ChatServer2 RPC

```bash
# 使用 ufw（Ubuntu）
sudo ufw allow 8080/tcp
sudo ufw allow 8090/tcp
sudo ufw allow 8091/tcp

# 或使用 iptables
sudo iptables -A INPUT -p tcp --dport 8080 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 8090 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 8091 -j ACCEPT
```

---

## 🗄️ 数据库初始化

### 13. MySQL 数据库设置

```sql
-- 创建数据库
CREATE DATABASE lyz CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- 创建远程访问用户
CREATE USER 'remote_admin'@'%' IDENTIFIED BY 'your_password';
GRANT ALL PRIVILEGES ON lyz.* TO 'remote_admin'@'%';
FLUSH PRIVILEGES;

-- 导入表结构（需要根据实际SQL文件执行）
USE lyz;
SOURCE your_schema.sql;
```

### 14. Redis 配置

```bash
# 编辑 Redis 配置
sudo vim /etc/redis/redis.conf

# 设置密码
requirepass YOUR_REDIS_PASSWORD

# 允许远程访问（如需）
bind 0.0.0.0

# 重启 Redis
sudo systemctl restart redis
```

---

## 🔍 故障排查

### 常见问题

1. **编译失败：找不到 Protobuf 头文件**
   ```bash
   # 重新生成代码
   protoc -I=src/XXX --cpp_out=src/XXX --grpc_out=src/XXX \
     --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) src/XXX/message.proto
   ```

2. **运行时找不到配置文件**
   - 确保 `config.ini` 与可执行文件在同一目录
   - 检查当前工作目录是否正确

3. **数据库连接失败**
   - 检查 MySQL 是否启动：`systemctl status mysql`
   - 验证网络连接：`telnet MYSQL_HOST PORT`
   - 确认用户名密码正确

4. **Redis 连接失败**
   - 检查 Redis 是否启动：`systemctl status redis`
   - 测试连接：`redis-cli -h HOST -p PORT -a PASSWORD ping`

5. **端口被占用**
   ```bash
   # 查看端口占用
   sudo lsof -i :PORT
   sudo netstat -tlnp | grep PORT
   
   # 杀死占用进程
   sudo kill -9 PID
   ```

---

## 📝 配置文件参数说明

### GateServer 配置

| 参数 | 说明 | 示例 |
|------|------|------|
| `Port` | 网关监听端口 | 8080 |
| `VerifyServer.Host` | 验证码服务地址 | 127.0.0.1 |
| `VerifyServer.Port` | 验证码服务端口 | 50051 |
| `Mysql.Host` | MySQL 主机地址 | 192.168.1.100 |
| `Mysql.Port` | MySQL 端口 | 3306 |
| `Mysql.User` | MySQL 用户名 | root |
| `Mysql.Passwd` | MySQL 密码 | password |
| `Mysql.Schema` | 数据库名称 | lyz |
| `Redis.Host` | Redis 主机地址 | 127.0.0.1 |
| `Redis.Port` | Redis 端口 | 6379 |
| `Redis.Passwd` | Redis 密码 | redis_password |
| `StatusServer.Host` | 状态服务器地址 | 127.0.0.1 |
| `StatusServer.Port` | 状态服务器端口 | 50052 |

### ChatServer 配置

| 参数 | 说明 | 示例 |
|------|------|------|
| `SelfServer.Name` | 服务器名称（唯一） | chatserver1 |
| `SelfServer.Host` | 监听地址 | 0.0.0.0 |
| `SelfServer.Port` | 业务端口 | 8090 |
| `SelfServer.RPCPort` | RPC 端口 | 50055 |
| `PeerServer.Servers` | 对等服务器列表 | chatserver2 |
| `[chatserver2].Host` | 对等服务器地址 | 192.168.1.101 |
| `[chatserver2].Port` | 对等服务器 RPC 端口 | 50056 |

### StatusServer 配置

| 参数 | 说明 | 示例 |
|------|------|------|
| `chatservers.Name` | 管理的服务器列表 | chatserver1,chatserver2 |
| `[chatserver1].Host` | 服务器1地址 | 127.0.0.1 |
| `[chatserver1].Port` | 服务器1业务端口 | 8090 |

### VerifyServer 配置（config.json）

| 参数 | 说明 | 示例 |
|------|------|------|
| `email.user` | 邮箱地址 | xxx@163.com |
| `email.pass` | 邮箱授权码（非登录密码） | ABCDEFGHIJKL |
| `mysql.host` | MySQL 主机地址 | 192.168.1.100 |
| `mysql.port` | MySQL 端口 | 3306 |
| `mysql.passwd` | MySQL 密码 | password |
| `redis.host` | Redis 主机地址 | 127.0.0.1 |
| `redis.port` | Redis 端口 | 6379 |
| `redis.passwd` | Redis 密码 | redis_password |

---

## 📌 注意事项

1. **配置文件安全**
   - ❌ 永远不要将 `config.ini` 或 `config.json` 提交到 Git
   - ✅ 使用 `.template` 文件作为配置模板
   - ✅ 生产环境使用强密码
   - ✅ 定期更换数据库、Redis 和邮箱授权码
   - ⚠️ **VerifyServer 的 `config.json` 包含邮箱授权码，务必妥善保管**

2. **网络配置**
   - 单服务器部署：使用 `127.0.0.1`
   - 多服务器部署：使用内网 IP 或公网 IP
   - 确保防火墙规则正确配置

3. **服务依赖**
   - 必须先启动 MySQL 和 Redis
   - 按顺序启动服务：StatusServer → GateServer → ChatServer
   - VerifyServer 可以独立启动

4. **日志管理**
   - 定期检查日志文件大小
   - 使用 logrotate 管理日志轮转
   - 监控错误日志及时发现问题

5. **VerifyServer 特别注意事项**
   - 邮箱授权码获取方式（以 163 邮箱为例）：
     1. 登录 163 邮箱网页版
     2. 进入「设置」→「POP3/SMTP/IMAP」
     3. 开启 SMTP 服务
     4. 获取授权码（不是登录密码）
   - 授权码应定期更换以提高安全性
   - 建议使用专用邮箱发送验证码，避免使用个人主邮箱

---

## 📞 技术支持

如遇到问题，请检查：
1. 所有依赖是否正确安装
2. 配置文件是否正确设置
3. 数据库和 Redis 是否正常运行
4. 防火墙端口是否开放
5. 日志文件中的错误信息

---

**祝部署顺利！** 🎉
