# TCP层的自定义协议设计及基于该协议的一个内网穿透聊天程序

## 文件组织结构
- inc/
    - client.hpp  包含Client类的定义
    - protocol.hpp  定义了协议内容及decode/encode package的方法
    - server.hpp   包含Server类、Channel类、Participant类的定义
- src/
    - client.cpp   Client类的实现文件
    - client_program.cpp   实现一个可执行的client程序
    - server.cpp   server.hpp对应的实现文件
    - server_program.cpp   实现一个可执行的server程序
    - protocol.cpp   协议的实现文件
- test/
    - test.cpp  针对的protocol的单元测试

## 所引用的外部库
- Boost.Asio  采用的是单线程异步IO模式
- glog  用于输出详细log信息
- gtest 单元测试

## 程序运行
1. 编译出client和server两个可执行程序
2. 运行server端： ./server 端口号
3. 运行多个client端： ./client 服务端的IP或域名 服务端的端口号
4. 输入: !create CHANNEL_NAME   创建一个CHANNEL
5. 另一个client输入：!join CHANNEL_NAME 加入指定的CHANNEL
6. 两端进入消息收发循环
7. 输入：!leave 退出当前CHANNEL
8. 其他命令： !list 列出当前Server上存在的所有CHANNEL

## 可优化的地方
- 引入openssl库，实现在公网环境下的加密通信
- 补充测试代码如性能测试, client及server的单元测试
