#include"UserManager.h"
#include <fstream>
#include <sstream>
#include <windows.h>

std::string UserManager::GetExecutablePath() const {
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string path(buffer);
	size_t lastSlash = path.find_last_of("\\/");
	if (lastSlash != std::string::npos) {
		return path.substr(0, lastSlash);
	}
	return ".";
}

UserManager::UserManager(const std::string& registryFile) : m_registryFile(GetExecutablePath() + "\\" + registryFile) {
	ReadRegistry(m_registryFile);
}

UserManager::~UserManager() {
	SaveRegistry(m_registryFile);
}

UserManager& UserManager::GetInstance() {
	static UserManager instance("registry.txt");
	return instance;
}

bool UserManager::SaveRegistry(const std::string& fileName) {
	std::ofstream file(fileName);
	if (!file) {
		std::cerr << "Failed to save registry file: " << fileName << std::endl;
		return false;
	}

	for (const auto& pair : m_registryCache) {
		file << pair.first << " " << pair.second.first << " " << pair.second.second << std::endl;
	}

	return true;
}

bool UserManager::Register(const std::string& username, const std::string& password, SOCKET socket) {
	// 获取下一个可用的用户ID，确保唯一性
	int userID = m_nextUserID++;
	
	// 再次验证ID不冲突（防御性编程）
	while (m_registryCache.count(userID)) {
		userID = m_nextUserID++;
	}
	
	m_registryCache[userID] = { username, password };
	
	// 保存到文件
	if (!SaveRegistry(m_registryFile)) {
		// 如果保存失败，回滚操作
		m_registryCache.erase(userID);
		std::string errorMsg = "Registration failed: Unable to save to registry file.\n";
		SendToSocket(socket, errorMsg);
		std::cout << "Registration failed: Unable to save to registry file." << std::endl;
		return false;
	}
	
	std::string successMsg = "Register success! Your ID is: " + std::to_string(userID) + "\n";
	SendToSocket(socket, successMsg);
	std::cout << "Register success! Your ID is: " << userID << std::endl;
	std::cout << "Username: " << username << " (Note: Usernames can be duplicated, but IDs are unique)" << std::endl;
	return true;
}

void UserManager::SendToSocket(SOCKET socket, const std::string& message) {
	send(socket, message.c_str(), static_cast<int>(message.size()), 0);
}

bool UserManager::Login(int id, const std::string& password, SOCKET socket) {
	if (!HasRegisted(id)) {
		std::string errorMsg = "This ID hasn't registered. Please register first.\n";
		SendToSocket(socket, errorMsg);
		std::cout << "This ID hasn't registered. Please register first." << std::endl;
		return false;
	}

	if (!CheckPassword(id, password)) {
		std::string errorMsg = "Incorrect password for ID " + std::to_string(id) + ", please re-enter it.\n";
		SendToSocket(socket, errorMsg);
		std::cout << "Incorrect password for ID " << id << ", please re-enter it." << std::endl;
		return false;
	}

	// 检查用户是否已经在线
	if (HasUser(id)) {
		std::string errorMsg = "User ID " + std::to_string(id) + " is already logged in.\n";
		SendToSocket(socket, errorMsg);
		std::cout << "User ID " << id << " is already logged in." << std::endl;
		return false;
	}

	const std::string& username = m_registryCache[id].first;
	AddConnectedUser(id, username, socket);
	return true;
}

void UserManager::AddConnectedUser(int id, const std::string& username, SOCKET socket) {
	User user(id, username, socket);
	
	// ID为主键，必须唯一
	m_usersByID.emplace(id, user);
	
	// 用户名可能重复，处理方式：
	// 1. 如果用户名已存在，我们仍然添加，但GetUser(username)只返回第一个匹配的
	// 2. 所有重要操作都应该基于ID进行
	auto existingByName = m_usersByName.find(username);
	if (existingByName != m_usersByName.end()) {
		std::cout << "Warning: Username '" << username << "' is already in use by another online user. "
				  << "Use ID " << id << " for unique identification." << std::endl;
	}
	m_usersByName.emplace(username, user);
	
	// 更新用户状态 - 只通过ID查找，确保准确性
	auto userIt = m_usersByID.find(id);
	if (userIt != m_usersByID.end()) {
		userIt->second.SetStatus(UserStatus::IN_LOBBY);
		userIt->second.SetRoom(-1);
	}
}

void UserManager::RemoveConnectedUser(int id) {
	auto it = m_usersByID.find(id);
	if (it != m_usersByID.end()) {
		std::string username = it->second.GetUsername();
		
		// 从ID映射中移除
		m_usersByID.erase(id);
		
		// 从用户名映射中移除对应的条目
		// 注意：如果有重名用户，需要找到正确的条目
		auto range = m_usersByName.equal_range(username);
		for (auto nameIt = range.first; nameIt != range.second; ++nameIt) {
			if (nameIt->second.GetID() == id) {
				m_usersByName.erase(nameIt);
				break;
			}
		}
	}
}

std::vector<User*> UserManager::GetOnlineUsers() const {
	std::vector<User*> users;
	for (auto& pair : m_usersByID) {
		users.push_back(const_cast<User*>(&pair.second));
	}
	return users;
}

bool UserManager::CancelAccount(int id, const std::string& password) {
	if (!HasRegisted(id)) {
		std::cout << "This ID hasn't registered. You can't cancel it." << std::endl;
		return false;
	}

	if (!CheckPassword(id, password)) {
		std::cout << "Incorrect password for ID " << id << ", you can't cancel account." << std::endl;
		return false;
	}

	// 如果用户当前在线，不允许注销
	if (HasUser(id)) {
		std::cout << "Cannot cancel account while user is online. Please logout first." << std::endl;
		return false;
	}

	m_registryCache.erase(id);
	
	// 立即保存到文件
	if (!SaveRegistry(m_registryFile)) {
		std::cout << "Warning: Account cancelled but failed to save changes to file." << std::endl;
	}

	std::cout << "Account ID " << id << " has been successfully cancelled." << std::endl;
	return true;
}

bool UserManager::ReadRegistry(const std::string& fileName) {
	std::ifstream file(fileName);
	if (!file) {
		std::cerr << "Registry file not found: " << fileName << ". Starting with empty registry." << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		int id;
		std::string username, password;
		if (iss >> id >> username >> password) {
			m_registryCache[id] = { username, password };
		}
	}

	// 正确计算下一个可用的用户ID
	m_nextUserID = CalculateNextUserID();
	
	std::cout << "Registry loaded: " << m_registryCache.size() << " users registered." << std::endl;
	std::cout << "Next available user ID: " << m_nextUserID << std::endl;

	return true;
}

int UserManager::CalculateNextUserID() const {
	if (m_registryCache.empty()) {
		return 1;
	}
	
	// 找到最大的ID并返回下一个可用ID
	int maxID = 0;
	for (const auto& pair : m_registryCache) {
		maxID = max(maxID, pair.first);
	}
	
	return maxID + 1;
}

bool UserManager::CheckPassword(int id, const std::string& password) {
	auto it = m_registryCache.find(id);
	if (it == m_registryCache.end()) {
		return false;
	}
	return it->second.second == password;
}

bool UserManager::HasRegisted(int id) const {
	return m_registryCache.count(id) > 0;
}

User* UserManager::GetUser(const std::string& username) {
	auto it = m_usersByName.find(username);
	if (it != m_usersByName.end()) {
		// 如果有多个同名用户，返回第一个找到的
		// 建议使用GetUser(id)来获取精确的用户
		std::cout << "Warning: Searching by username may return unexpected results if multiple users have the same name. Consider using ID instead." << std::endl;
		return &it->second;
	}
	return nullptr;
}

User* UserManager::GetUser(int id) {
	auto it = m_usersByID.find(id);
	if (it != m_usersByID.end()) {
		return &it->second;
	}
	return nullptr;
}

bool UserManager::HasUser(const std::string& username) const {
	return m_usersByName.find(username) != m_usersByName.end();
}

bool UserManager::HasUser(int id) const {
	return m_usersByID.find(id) != m_usersByID.end();
}

int UserManager::GetNextUserID() {
	return m_nextUserID;
}