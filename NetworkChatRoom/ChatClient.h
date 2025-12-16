#pragma once
#include <iostream>
#include <string>
#include <thread>

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// 默认端口号与缓冲区大小（应与服务器一致）
constexpr int PORT = 8888;
constexpr int BUFFER_SIZE = 1024;

class ChatClient {
public:

private:
};