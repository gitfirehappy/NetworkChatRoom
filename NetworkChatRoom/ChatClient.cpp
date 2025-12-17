// 客户端实现
#include"ChatClient.h"

ChatClient::ChatClient(const std::string& serverIp, int port)
    : m_serverIp(serverIp), m_port(port), m_socket(INVALID_SOCKET), m_isRunning(false), m_exitMessage("EXIT") {
    // 初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
}

ChatClient::~ChatClient() {
    Stop();
    WSACleanup();
}

void ChatClient::Start() {
    if (m_isRunning) return;

    // 创建socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        throw std::runtime_error("socket failed");
    }

    // 连接服务器
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    inet_pton(AF_INET, m_serverIp.c_str(), &(addr.sin_addr));

    if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(m_socket);
        throw std::runtime_error("connect failed");
    }

    m_isRunning = true;
    m_receiveThread = std::thread(&ChatClient::ReceiveLoop, this);
    SendLoop();
}

void ChatClient::Stop() {
    m_isRunning = false;
    closesocket(m_socket);
    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }
}

void ChatClient::ReceiveLoop() {
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while (m_isRunning) {
        bytesReceived = recv(m_socket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            std::cout << "Server: " << std::string(buffer,bytesReceived) << "\n";
        }
        else if (bytesReceived == 0) {
            std::cout << "Server disconnected\n";
            break;
        }
        else {
            std::cerr << "recv failed: " << WSAGetLastError() << "\n";
            break;
        }
    }
}

void ChatClient::SendLoop() {
    std::string message;
    while (m_isRunning) {
        std::getline(std::cin, message);
        if (message == m_exitMessage) {
            m_isRunning = false;
            break;
        }
        send(m_socket, message.c_str(), static_cast<int>(message.size()), 0);
    }
}
