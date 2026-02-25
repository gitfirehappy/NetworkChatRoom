#include"UserManager.h"
#include <fstream>
#include <sstream>

UserManager::UserManager(const std::string& registryFile) : m_registryFile(registryFile) {
	ReadRegistry(registryFile);
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

bool UserManager::Register(const std::string& username, const std::string& password) {
	int userID = m_nextUserID++;
	m_registryCache[userID] = { username, password };
	// 保存到文件
	SaveRegistry(m_registryFile);
	
	std::cout << "Register success! Your ID is: " << userID << std::endl;
	return true;
}

bool UserManager::Login(int id, const std::string& password, SOCKET socket) {
	if (!HasRegisted(id)) {
		// 提示先注册
		std::cout << "This ID hasn't registered. Please register first." << std::endl;
		return false;
	}

	if (!CheckPassword(id, password)) {
		std::cout << "Incorrect password for ID, please re-enter it." << std::endl;
		return false;
	}

	const std::string& username = m_registryCache[id].first;
	AddConnectedUser(id, username, socket);
	return true;
}

void UserManager::AddConnectedUser(int id, const std::string& username, SOCKET socket) {
	User user(id, username, socket);
	m_usersByID.emplace(id, user);
	m_usersByName.emplace(username, user);
	
	// 更新用户状态
	auto userByIdIt = m_usersByID.find(id);
	if (userByIdIt != m_usersByID.end()) {
		userByIdIt->second.SetStatus(UserStatus::IN_LOBBY);
		userByIdIt->second.SetRoom(-1);
	}

	auto userByNameIt = m_usersByName.find(username);
	if (userByNameIt != m_usersByName.end()) {
		userByNameIt->second.SetStatus(UserStatus::IN_LOBBY);
		userByNameIt->second.SetRoom(-1);
	}
}

void UserManager::RemoveConnectedUser(int id) {
	auto it = m_usersByID.find(id);
	if (it != m_usersByID.end()) {
		std::string username = it->second.GetUsername();
		m_usersByID.erase(id);
		m_usersByName.erase(username);
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
		std::cout << "This ID hasn't registed.You can't cancel it";
		return false;
	}

	if (!CheckPassword(id, password)) {
		std::cout << "Incorrect password to ID, you can't cancel account";
		return false;
	}

	// 只允许在注册阶段注销，所以不用调整当前Users管理表

	m_registryCache.erase(id);
	// 同理，等退出再覆写文件

	return true;
}

bool UserManager::ReadRegistry(const std::string& fileName) {
	std::ifstream file(fileName);
	if (!file) {
		std::cerr << "Failed to open registry file: " << fileName << std::endl;
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

	if (!m_registryCache.empty()) {
		m_nextUserID = m_registryCache.cbegin()->first + 1; // TODO:不一定是最大的
	}

	return true;
}

bool UserManager::CheckPassword(int id, const std::string& password) {
	return m_registryCache[id].second == password;
}

bool UserManager::HasRegisted(int id) const {
	return m_registryCache.count(id);
}

User* UserManager::GetUser(const std::string& username) {
	auto it = m_usersByName.find(username);
	if (it != m_usersByName.end()) {
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