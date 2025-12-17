// 服务器实现
#include"ChatServer.h"
#include<vector>

ChatServer::ChatServer(int port)
	: m_port(port), m_listenSocket(INVALID_SOCKET), m_isRunning(false), m_clientCounter(0) {
	// 初始化Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		throw std::runtime_error("WSAStartup failed");
	}
}

ChatServer::~ChatServer() {
	if (m_isRunning) {
		Stop();
	}
	WSACleanup();
}

void ChatServer::Start() {
	if (m_isRunning) return;

	// 创建监听套接字
	m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_listenSocket == INVALID_SOCKET) {
		throw std::runtime_error("socket failed");
	}

	// 绑定地址
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(m_port);

	if (bind(m_listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		closesocket(m_listenSocket);
		throw std::runtime_error("bind failed");
	}

	// 监听连接
	if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		closesocket(m_listenSocket);
		throw std::runtime_error("listen failed");
	}

	std::cout << "Server started on port " << m_port << "\n";
	m_isRunning = true;

	// 启动接收线程
	m_acceptThread = std::thread(&ChatServer::AcceptLoop, this);
}

void ChatServer::Stop() {
	m_isRunning = false;
	closesocket(m_listenSocket);
	m_acceptThread.join();

	// 关闭所有客户端连接
	// 拷贝确保线程安全
	std::vector<SOCKET> clientCopy; 
	{
		std::lock_guard<std::mutex> lock(m_clientsMutex);
		for (auto& pair : m_clients) {
			clientCopy.emplace_back(pair.first);
		}
		m_clients.clear();
	}

	for (SOCKET client : clientCopy) {
		closesocket(client);
	}
}

void ChatServer::AcceptLoop() {
	while (m_isRunning) {
		sockaddr_in clientAddr;
		int addrLen = sizeof(clientAddr);
		SOCKET clientSocket = accept(m_listenSocket, (sockaddr*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET) {
			if (WSAGetLastError() != WSAEINTR) {
				std::cerr << "accet failed: " << WSAGetLastError() << "\n";
			}
			continue;
		}

		// 为新客户端分配唯一ID
		int clientID = m_clientCounter++;
		{
			std::lock_guard<std::mutex> lock(m_clientsMutex);
			m_clients[clientSocket] = clientID;
		}
		char clientIpStr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIpStr, INET_ADDRSTRLEN);
		std::cout << "Client connected: " << clientIpStr 
			<< "(ID: " << clientID << ")\n";

		// 启动独立线程处理客户端
		std::thread(&ChatServer::ClientHandler, this, clientSocket).detach();
	}
}

void ChatServer::ClientHandler(SOCKET clientSocket) {
	char buffer[BUFFER_SIZE];
	int bytesReceived;

	int clientID;
	{
		std::lock_guard<std::mutex> lock(m_clientsMutex);
		clientID = m_clients[clientSocket];
	}

	while ((bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
		std::string message(buffer, bytesReceived);
		std::cout << "Client[" << clientID << "]: " << message << "\n";
		
		std::string formattedMessage = "[" + std::to_string(clientID) + "] " + message;
		BroadcastMessage(formattedMessage, clientSocket);
	}

	// 客户端断开连接
	{
		std::lock_guard<std::mutex> lock(m_clientsMutex);
		m_clients.erase(clientSocket);
	}
	std::cout << "Client[" << clientID << "] disconnected\n";
	closesocket(clientSocket);
}

void ChatServer::BroadcastMessage(const std::string& message, SOCKET excludeSocket) {
	std::vector<SOCKET> clientCopy;
	{
		std::lock_guard<std::mutex> lock(m_clientsMutex);
		for (auto& pair : m_clients) {
			if (pair.first != excludeSocket) { // 跳过排除的客户端
				clientCopy.emplace_back(pair.first);
			}
		}
	}

	for (SOCKET client : clientCopy) {
		send(client, message.c_str(), static_cast<int>(message.size()), 0);
	}
}
