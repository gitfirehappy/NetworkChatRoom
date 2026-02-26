// 服务器实现
#include"ChatServer.h"
#include<vector>
#include"UserManager.h"
#include"RoomManager.h"
#include"Logger.h"
#include<sstream>

ChatServer::ChatServer(int port)
	: m_port(port), m_listenSocket(INVALID_SOCKET), m_isRunning(false), m_clientCounter(0) {
	// 初始化 Winsock
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
	Logger::GetInstance().Log("Server started on port " + std::to_string(m_port));
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
				std::cerr << "accept failed: " << WSAGetLastError() << "\n";
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

	// 发送欢迎消息和注册/登录提示
		std::string welcomeMsg = COLOR_GREEN + "Welcome to Chat Server!\n" + COLOR_RESET + 
			"Please register(" + COLOR_CYAN + "/register <username> <password>" + COLOR_RESET + ") or login(" + COLOR_CYAN + "/login <id> <password>" + COLOR_RESET + ")\n";
		send(clientSocket, welcomeMsg.c_str(), static_cast<int>(welcomeMsg.size()), 0);

	while ((bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
		std::string message(buffer, bytesReceived);
		// 只有当消息末尾有换行符时才移除
		if (!message.empty() && message.back() == '\n') {
			message.pop_back();
		}
		// 处理可能存在的回车符
		if (!message.empty() && message.back() == '\r') {
			message.pop_back();
		}
		
		std::cout << "Client[" << clientID << "]: " << message << "\n";
		
		// 检查是否为命令
		if (message[0] == '/') {
			if (ProcessCommand(message, clientSocket)) {
				continue; // 如果是有效命令，则跳过后续处理
			}
		}
		
		// 检查用户是否已登录
		UserManager& userManager = UserManager::GetInstance();
		User* user = nullptr;
		for (auto& pair : userManager.GetOnlineUsers()) {
			if (pair->GetSocket() == clientSocket) {
				user = pair;
				break;
			}
		}
		
		if (!user) {
			std::string errorMsg = "Please login first!\n";
			send(clientSocket, errorMsg.c_str(), static_cast<int>(errorMsg.size()), 0);
			continue;
		}
		
		// 根据用户状态决定消息处理方式
		if (user->GetUserStatus() == UserStatus::IN_ROOM) {
			// 如果用户在房间内，只向房间内成员广播
			int roomID = user->GetRoomID();
			std::string formattedMessage = COLOR_YELLOW + "[ROOM-" + std::to_string(roomID) + "] " + COLOR_RESET + 
				COLOR_BLUE + "[" + user->GetUsername() + "]" + COLOR_RESET + ": " + message;
			BroadcastToRoom(formattedMessage, roomID, clientSocket);
		} else {
			// 否则向大厅所有用户广播
			std::string formattedMessage = COLOR_CYAN + "[LOBBY] " + COLOR_RESET + 
				COLOR_BLUE + "[" + user->GetUsername() + "]" + COLOR_RESET + ": " + message;
			BroadcastMessage(formattedMessage, clientSocket);
		}
	}

	// 客户端断开连接
	{
		std::lock_guard<std::mutex> lock(m_clientsMutex);
		m_clients.erase(clientSocket);
	}
	
	// 从用户管理器中移除用户
	UserManager& userManager = UserManager::GetInstance();
	for (auto& pair : userManager.GetOnlineUsers()) {
		if (pair->GetSocket() == clientSocket) {
			userManager.RemoveConnectedUser(pair->GetID());
			break;
		}
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

void ChatServer::BroadcastToRoom(const std::string& message, int roomID, SOCKET senderSocket) {
	RoomManager& roomManager = RoomManager::GetInstance();
	Room* room = roomManager.GetRoom(roomID);
	if (!room) return;
	
	const std::unordered_set<int>& users = room->GetUsers();
	UserManager& userManager = UserManager::GetInstance();
	
	for (int userID : users) {
		User* user = userManager.GetUser(userID);
		if (user && user->GetSocket() != senderSocket) {
			send(user->GetSocket(), message.c_str(), static_cast<int>(message.size()), 0);
		}
	}
}



bool ChatServer::ProcessCommand(const std::string& message, SOCKET clientSocket) {
	std::istringstream iss(message);
	std::string command;
	iss >> command;
	
	UserManager& userManager = UserManager::GetInstance();
	User* currentUser = nullptr;
	
	// 找到当前用户
	for (auto& user : userManager.GetOnlineUsers()) {
		if (user->GetSocket() == clientSocket) {
			currentUser = user;
			break;
		}
	}
	
	// 处理不需要登录的命令
	if (command == "/register") {
		std::string username, password;
		iss >> username >> password;
		
		if (userManager.Register(username, password)) {
			std::string response = COLOR_GREEN + "Registration successful!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command executed: /register " + username);
		} else {
			std::string response = COLOR_RED + "Registration failed!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /register " + username);
		}
		return true;
	}
	
	// 处理需要登录的命令
	if (!currentUser && command != "/login") {
		std::string response = COLOR_YELLOW + "Please login first!\n" + COLOR_RESET;
		send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
		Logger::GetInstance().Log("Command failed: " + command + " (User not logged in)");
		return true;
	}
	
	// 登录命令
	if (command == "/login") {
		int id;
		std::string password;
		iss >> id >> password;
		
		if (userManager.Login(id, password, clientSocket)) {
			std::string response = COLOR_GREEN + "Login successful! Welcome to the lobby.\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command executed: /login " + std::to_string(id));
		} else {
			std::string response = COLOR_RED + "Login failed!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /login " + std::to_string(id));
		}
		return true;
	}
	
	// 如果用户没有登录，但尝试执行需要登录的命令
	if (!currentUser) {
		Logger::GetInstance().Log("Command failed: " + command + " (User not logged in)");
		return false; // 不是有效命令或用户未登录
	}
	
	// 退出服务器命令
	if (command == "/Exit_Server") {
		std::string response = COLOR_YELLOW + "Disconnecting from server...\n" + COLOR_RESET;
		send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
		Logger::GetInstance().Log("Command executed: /Exit_Server by user " + std::to_string(currentUser->GetID()));
		closesocket(clientSocket); // 直接关闭连接
		return true;
	}
	
	// 退出大厅命令
	if (command == "/Exit_Lobby") {
		currentUser->SetStatus(UserStatus::IN_LOBBY);
		std::string response = COLOR_YELLOW + "Returned to login stage. Please login again.\n" + COLOR_RESET;
		send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
		userManager.RemoveConnectedUser(currentUser->GetID());
		Logger::GetInstance().Log("Command executed: /Exit_Lobby by user " + std::to_string(currentUser->GetID()));
		return true;
	}
	
	// 退出房间命令
	if (command == "/Exit_Room") {
		if (currentUser->GetUserStatus() != UserStatus::IN_ROOM) {
			std::string response = COLOR_RED + "You are not in a room!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /Exit_Room by user " + std::to_string(currentUser->GetID()) + " (Not in room)");
			return true;
		}
		
		int roomID = currentUser->GetRoomID();
		RoomManager& roomManager = RoomManager::GetInstance();
		roomManager.LeaveRoom(roomID, currentUser->GetID());
		currentUser->SetStatus(UserStatus::IN_LOBBY);
		currentUser->SetRoom(-1);
		
		std::string response = COLOR_GREEN + "Exited room " + std::to_string(roomID) + ". Returned to lobby.\n" + COLOR_RESET;
		send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
		Logger::GetInstance().Log("Command executed: /Exit_Room by user " + std::to_string(currentUser->GetID()) + " from room " + std::to_string(roomID));
		return true;
	}
	
	// 踢出用户命令（仅房主可用）
	if (command == "/Kick_Out") {
		if (currentUser->GetUserStatus() != UserStatus::IN_ROOM) {
			std::string response = COLOR_RED + "You are not in a room!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /Kick_Out by user " + std::to_string(currentUser->GetID()) + " (Not in room)");
			return true;
		}
		
		int roomID = currentUser->GetRoomID();
		RoomManager& roomManager = RoomManager::GetInstance();
		Room* room = roomManager.GetRoom(roomID);
		
		if (!room || room->GetOwnerID() != currentUser->GetID()) {
			std::string response = COLOR_RED + "You are not the room owner!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /Kick_Out by user " + std::to_string(currentUser->GetID()) + " (Not room owner)");
			return true;
		}
		
		int userID;
		iss >> userID;
		
		User* userToKick = userManager.GetUser(userID);
		if (!userToKick || !room->ContainsUser(userID)) {
			std::string response = COLOR_RED + "User not found in this room!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /Kick_Out by user " + std::to_string(currentUser->GetID()) + " (User not found in room)");
			return true;
		}
		
		roomManager.LeaveRoom(roomID, userID);
		userToKick->SetStatus(UserStatus::IN_LOBBY);
		userToKick->SetRoom(-1);
		
		std::string response = COLOR_YELLOW + "User " + std::to_string(userID) + " has been kicked out of the room.\n" + COLOR_RESET;
		send(userToKick->GetSocket(), response.c_str(), static_cast<int>(response.size()), 0);
		send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
		Logger::GetInstance().Log("Command executed: /Kick_Out by user " + std::to_string(currentUser->GetID()) + " kicked user " + std::to_string(userID) + " from room " + std::to_string(roomID));
		return true;
	}
	
	// 查看房间用户列表命令
	if (command == "/user_in_room") {
		if (currentUser->GetUserStatus() != UserStatus::IN_ROOM) {
			std::string response = COLOR_RED + "You are not in a room!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /user_in_room by user " + std::to_string(currentUser->GetID()) + " (Not in room)");
			return true;
		}
		
		int roomID = currentUser->GetRoomID();
		RoomManager& roomManager = RoomManager::GetInstance();
		Room* room = roomManager.GetRoom(roomID);
		
		if (!room) {
			std::string response = COLOR_RED + "Room not found!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /user_in_room by user " + std::to_string(currentUser->GetID()) + " (Room not found)");
			return true;
		}
		
		std::string userList = COLOR_CYAN + "Users in room " + std::to_string(roomID) + ":\n" + COLOR_RESET;
		for (int userID : room->GetUsers()) {
			User* user = userManager.GetUser(userID);
			if (user) {
				userList += COLOR_BLUE + "- " + std::to_string(userID) + ": " + user->GetUsername() + COLOR_RESET;
				if (user->GetID() == room->GetOwnerID()) {
					userList += COLOR_GREEN + " (Owner)" + COLOR_RESET;
				}
				userList += "\n";
			}
		}
		
		send(clientSocket, userList.c_str(), static_cast<int>(userList.size()), 0);
		Logger::GetInstance().Log("Command executed: /user_in_room by user " + std::to_string(currentUser->GetID()) + " in room " + std::to_string(roomID));
		return true;
	}
	
	// 创建房间命令
	if (command == "/create_room") {
		std::string roomName, password;
		int maxUsers = 10;
		iss >> roomName >> password >> maxUsers;
		
		if (maxUsers < 2 || maxUsers > 10) {
			maxUsers = 10; // 默认值
		}
		
		RoomManager& roomManager = RoomManager::GetInstance();
		Room* newRoom = roomManager.CreateRoom(roomName, currentUser->GetID(), password, maxUsers);
		
		if (newRoom) {
			currentUser->SetStatus(UserStatus::IN_ROOM);
			currentUser->SetRoom(newRoom->GetID());
			
			std::string response = COLOR_GREEN + "Room created successfully! Room ID: " + std::to_string(newRoom->GetID()) + "\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command executed: /create_room by user " + std::to_string(currentUser->GetID()) + " created room " + std::to_string(newRoom->GetID()) + " (" + roomName + ")");
		} else {
			std::string response = COLOR_RED + "Failed to create room!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /create_room by user " + std::to_string(currentUser->GetID()) + " (" + roomName + ")");
		}
		return true;
	}
	
	// 查看房间列表命令
	if (command == "/room_list") {
		RoomManager& roomManager = RoomManager::GetInstance();
		std::vector<Room*> rooms = roomManager.GetAllRooms();
		
		std::string roomList = COLOR_CYAN + "Available rooms:\n" + COLOR_RESET;
		if (rooms.empty()) {
			roomList += COLOR_YELLOW + "No rooms available.\n" + COLOR_RESET;
		} else {
			for (Room* room : rooms) {
				User* owner = userManager.GetUser(room->GetOwnerID());
				roomList += COLOR_BLUE + "- Room ID: " + std::to_string(room->GetID()) + 
					   ", Name: " + room->GetName() +
					   ", Owner: " + (owner ? owner->GetUsername() : "Unknown") +
					   ", Users: " + std::to_string(room->GetUserCount()) + "/" + std::to_string(room->GetMaxUsers()) + "\n" + COLOR_RESET;
			}
		}
		
		send(clientSocket, roomList.c_str(), static_cast<int>(roomList.size()), 0);
		Logger::GetInstance().Log("Command executed: /room_list by user " + std::to_string(currentUser->GetID()));
		return true;
	}
	
	// 进入房间命令
	if (command == "/enter_room") {
		int roomID;
		std::string password;
		iss >> roomID >> password;
		
		if (currentUser->GetUserStatus() == UserStatus::IN_ROOM) {
			std::string response = COLOR_RED + "You are already in a room! Please exit first.\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /enter_room by user " + std::to_string(currentUser->GetID()) + " (Already in room)");
			return true;
		}
		
		RoomManager& roomManager = RoomManager::GetInstance();
		if (!roomManager.IsRoomExists(roomID)) {
			std::string response = COLOR_RED + "Room does not exist!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /enter_room by user " + std::to_string(currentUser->GetID()) + " (Room " + std::to_string(roomID) + " does not exist)");
			return true;
		}
		
		if (roomManager.JoinRoom(roomID, currentUser->GetID(), password)) {
			currentUser->SetStatus(UserStatus::IN_ROOM);
			currentUser->SetRoom(roomID);
			
			std::string response = COLOR_GREEN + "Successfully joined room " + std::to_string(roomID) + "!\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command executed: /enter_room by user " + std::to_string(currentUser->GetID()) + " joined room " + std::to_string(roomID));
		} else {
			std::string response = COLOR_RED + "Failed to join room! Wrong password or room is full.\n" + COLOR_RESET;
			send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
			Logger::GetInstance().Log("Command failed: /enter_room by user " + std::to_string(currentUser->GetID()) + " (Room " + std::to_string(roomID) + ")");
		}
		return true;
	}
	
	// 帮助命令
	if (command == "/help") {
		std::string helpText = 
			COLOR_MAGENTA + "Available commands:\n" + COLOR_RESET
			+ COLOR_BLUE + "/register <username> <password>" + COLOR_RESET + " - Register a new account\n"
			+ COLOR_BLUE + "/login <id> <password>" + COLOR_RESET + " - Login to your account\n"
			+ COLOR_BLUE + "/Exit_Server" + COLOR_RESET + " - Exit the server\n"
			+ COLOR_BLUE + "/Exit_Lobby" + COLOR_RESET + " - Return to login stage\n"
			+ COLOR_BLUE + "/Exit_Room" + COLOR_RESET + " - Exit current room\n"
			+ COLOR_BLUE + "/Kick_Out <user_id>" + COLOR_RESET + " - Kick user from room (owner only)\n"
			+ COLOR_BLUE + "/user_in_room" + COLOR_RESET + " - Show users in current room\n"
			+ COLOR_BLUE + "/create_room <name> <password> <max_users>" + COLOR_RESET + " - Create a new room\n"
			+ COLOR_BLUE + "/room_list" + COLOR_RESET + " - Show all available rooms\n"
			+ COLOR_BLUE + "/enter_room <room_id> <password>" + COLOR_RESET + " - Enter a room\n"
			+ COLOR_BLUE + "/help" + COLOR_RESET + " - Show this help message\n"
			+ COLOR_BLUE + "/clear_screen" + COLOR_RESET + " - Clear screen (client side)\n";
		
		send(clientSocket, helpText.c_str(), static_cast<int>(helpText.size()), 0);
		Logger::GetInstance().Log("Command executed: /help by user " + std::to_string(currentUser->GetID()));
		return true;
	}
	
	// 清屏命令（发送指令让客户端清屏）
	if (command == "/clear_screen") {
		std::string clearCmd = "\033[2J\033[H"; // ANSI 转义序列清屏
		send(clientSocket, clearCmd.c_str(), static_cast<int>(clearCmd.size()), 0);
		Logger::GetInstance().Log("Command executed: /clear_screen by user " + std::to_string(currentUser->GetID()));
		return true;
	}
	
	Logger::GetInstance().Log("Command unknown: " + command);
	return false; // 不是有效命令
}
