#pragma once
#include<iostream>
#include<vector>
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

class ChatServer {
public:
    explicit ChatServer(int port = PORT);   // 构造函数，显式
    ~ChatServer();                          // 析构函数
    void Start();                           // 运行服务器
    void Stop();                            // 停止运行服务器
private:
    // 接收新连接的主循环
    // 独立线程后台运行，持续等待新客户端
    void AcceptLoop();  
    
    // 客户端消息处理函数
    void ClientHandler(SOCKET clientSocket);    
    
    // 广播消息给所有在线客户端
    void BroadcastMessage(const std::string& message, SOCKET excludeSocket = INVALID_SOCKET);
    
    // 成员变量
    int m_port;                     // 服务器监听端口
    SOCKET m_listenSocket;          // 监听socket
    std::vector<SOCKET> m_clients;  // 所有已连接的客户端Socket
    std::mutex m_clientsMutex;      // 保护m_clients 的互斥锁
    bool m_isRunning;               // 标记服务器是否在运行
    std::thread m_acceptThread;     // 接收连接的后台线程
};


