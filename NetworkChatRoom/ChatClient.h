#pragma once

#include"ChatServer.h"
#include <iostream>
#include <string>
#include <thread>

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// 默认端口号与缓冲区大小（服务器头文件已定义）

/**
 * @brief 聊天客户端类
 *
 * 该类封装了一个连接到聊天服务器的 TCP 客户端。
 * - 主线程负责从控制台读取用户输入并发送
 * - 后台线程持续接收服务器广播的消息并打印
 * - 支持通过输入 "exit" 退出
 */
class ChatClient {
public:
    /**
    * @brief 构造函数
    * @param serverIp 服务器 IP 地址，默认为本地回环地址 "127.0.0.1"
    * @param port 服务器端口，默认为 8888
    */
    explicit ChatClient(const std::string& serverIp = "127.0.0.1", int port = PORT);

    /**
     * @brief 析构函数
     * 自动停止客户端并清理 Winsock 资源
     */
    ~ChatClient();

    /**
     * @brief 启动客户端
     * 连接到服务器，并启动接收线程
     * 主线程进入消息发送循环（从 std::cin 读取）
     */
    void Start();

    /**
     * @brief 停止客户端
     * 关闭套接字，停止接收线程
     */
    void Stop();
private:
    /**
    * @brief 接收消息的后台线程函数
    * 持续调用 recv() 接收服务器广播，并打印到控制台
    */
    void ReceiveLoop();

    /**
     * @brief 发送消息的主线程函数
     * 从标准输入读取消息，发送给服务器
     * 输入 "exit" 时退出
     */
    void SendLoop();

    // 成员变量
    std::string m_serverIp;   ///< 服务器 IP 地址
    int m_port;               ///< 服务器端口
    SOCKET m_socket;          ///< 与服务器通信的套接字
    bool m_isRunning;         ///< 标记客户端是否正在运行
    std::thread m_receiveThread; ///< 接收消息的后台线程
    std::string m_exitMessage;   ///< 退出连接消息
};