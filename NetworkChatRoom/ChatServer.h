#pragma once
#include<iostream>
#include<map>
#include<string>
#include<thread>
#include<mutex>
#include<algorithm>

// Winsock头文件
#include<WinSock2.h>
#include<WS2tcpip.h>

// 告诉链接器链接 ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

// 默认端口号与缓冲区大小
constexpr int PORT = 8888;
constexpr int BUFFER_SIZE = 1024;

/**
 * @brief 聊天服务器类
 *
 * 该类封装了一个基于 TCP 的多客户端聊天服务器。
 * - 使用单监听套接字接收新连接
 * - 每个客户端由独立线程处理消息收发
 * - 所有收到的消息会广播给其他所有在线客户端
 * - 线程安全：使用互斥锁保护客户端列表
 */
class ChatServer {
public:
    /**
     * @brief 构造函数
     * @param port 服务器监听的端口号，默认为 8888
     */
    explicit ChatServer(int port = PORT);

    /**
     * @brief 析构函数
     * 自动清理资源（关闭套接字、停止线程、调用 WSACleanup）
     */
    ~ChatServer();

    /**
     * @brief 启动服务器
     * 初始化监听套接字并开始接受客户端连接
     * 若已启动则直接返回
     * @throws std::runtime_error 若初始化失败（如 bind/listen 失败）
     */
    void Start();

    /**
     * @brief 停止服务器
     * 关闭监听套接字，断开所有客户端连接，并等待接收线程结束
     */
    void Stop();

private:
    /**
     * @brief 接受新连接的主循环
     * 在独立线程中运行，持续调用 accept() 等待新客户端
     */
    void AcceptLoop();

    /**
     * @brief 客户端消息处理函数
     * 在独立线程中运行，负责从指定客户端接收消息并广播
     * @param clientSocket 已连接的客户端套接字
     */
    void ClientHandler(SOCKET clientSocket);

    /**
     * @brief 广播消息给所有在线客户端（可排除发送者）
     * @param message 要广播的字符串消息
     * @param excludeSocket 可选参数：不向此套接字发送（通常为消息发送者）
     */
    void BroadcastMessage(const std::string& message, SOCKET excludeSocket = INVALID_SOCKET);

    // 成员变量
    int m_port;                     ///< 服务器监听端口
    SOCKET m_listenSocket;          ///< 监听套接字（用于 accept 新连接）
    std::map<SOCKET,int> m_clients;  ///< 当前所有已连接的客户端套接字 -> 客户端编号
    std::mutex m_clientsMutex;      ///< 保护 m_clients 的互斥锁（线程安全）
    bool m_isRunning;               ///< 标记服务器是否正在运行
    std::thread m_acceptThread;     ///< 接受连接的后台线程
    int m_clientCounter;            ///< 客户端计数器
};


