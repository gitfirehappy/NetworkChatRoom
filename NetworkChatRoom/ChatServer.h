#pragma once
#include<iostream>
#include<unordered_map>
#include<string>
#include<thread>
#include<mutex>
#include<algorithm>

// Winsock 头文件
#include<WinSock2.h>
#include<WS2tcpip.h>

// 告诉链接器链接 ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

// 默认端口号与缓冲区大小
constexpr int PORT = 8888;
constexpr int BUFFER_SIZE = 1024;

// ANSI 颜色代码
const std::string COLOR_RESET = "\033[0m";
const std::string COLOR_RED = "\033[31m";
const std::string COLOR_GREEN = "\033[32m";
const std::string COLOR_YELLOW = "\033[33m";
const std::string COLOR_BLUE = "\033[34m";
const std::string COLOR_MAGENTA = "\033[35m";
const std::string COLOR_CYAN = "\033[36m";

/**
 * 聊天服务器类
 *
 * 该类封装了一个基于 TCP 的多客户端聊天服务器。
 * - 使用单监听套接字接收新连接
 * - 每个客户端由独立线程处理消息收发
 * - 支持注册登录、房间聊天、私聊等功能
 * - 线程安全：使用互斥锁保护客户端列表
 */
class ChatServer {
public:
    /**
     * 构造函数
     * port 服务器监听的端口号，默认为 8888
     */
    explicit ChatServer(int port = PORT);

    /**
     * 析构函数
     * 自动清理资源（关闭套接字、停止线程、调用 WSACleanup）
     */
    ~ChatServer();

    /**
     * 启动服务器
     * 初始化监听套接字并开始接收客户端连接
     * 若已启动则直接返回
     * std::runtime_error 若初始化失败（如 bind/listen 失败）
     */
    void Start();

    /**
     * 停止服务器
     * 关闭监听套接字，断开所有客户端连接，并等待接收线程结束
     */
    void Stop();

private:
    /**
     * 接受新连接的主循环
     * 在独立线程中运行，持续调用 accept() 等待新客户端
     */
    void AcceptLoop();

    /**
     * 客户端消息处理函数
     * 在独立线程中运行，负责从指定客户端接收消息并广播
     * clientSocket 已连接的客户端套接字
     */
    void ClientHandler(SOCKET clientSocket);

    /**
     * 广播消息给所有在线客户端（可排除发送者）
     * message 要广播的字符串消息
     * excludeSocket 可选参数：不向此套接字发送（通常为消息发送者）
     */
    void BroadcastMessage(const std::string& message, SOCKET excludeSocket = INVALID_SOCKET);

    /**
     * 向特定房间广播消息
     * message 要广播的字符串消息
     * roomID 房间ID
     * senderSocket 发送者套接字（不向其发送）
     */
    void BroadcastToRoom(const std::string& message, int roomID, SOCKET senderSocket = INVALID_SOCKET);

    

    /**
     * 处理客户端命令
     * message 客户端发送的消息
     * clientSocket 客户端套接字
     */
    bool ProcessCommand(const std::string& message, SOCKET clientSocket);

    // 成员变量
    int m_port;                     // 服务器监听端口
    SOCKET m_listenSocket;          // 监听套接字（用于 accept 新连接）
    std::unordered_map<SOCKET, int> m_clients; // 当前所有已连接客户端套接字 -> 客户端编号
    std::mutex m_clientsMutex;      // 保护 m_clients 的互斥锁（线程安全）
    bool m_isRunning;               // 标记服务器是否正在运行
    std::thread m_acceptThread;     // 接受连接的后台线程
    int m_clientCounter;            // 客户端计数器
};


