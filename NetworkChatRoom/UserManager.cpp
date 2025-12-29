#include"UserManager.h"

bool UserManager::Register(const std::string& username, const std::string& password) {
	int userID = m_nextUserID++;
	m_registryCache[userID] = { username,password };
	// 这里不做文件保存，等退出前自动做一次保存，性能友好

	std::cout << "Regist success";
	return true;
}

bool UserManager::Login(int id, const std::string& password) {
	if (!HasRegisted(id)) {
		// 提示先注册
		std::cout << "This ID hasn't registed.Please regist first";
		return false;
	}

	if (!CheckPassword(id, password)) {
		std::cout << "Incorrect password to ID, please re-enter it";
		return false;
	}

	const std::string username = m_registryCache[id].first;
	User user(id, username, INVALID_SOCKET);
	m_usersByID[id] = user;
	m_usersByName[username] = user;
	user.SetStatus(UserStatus::IN_LOBBY);
	return true;
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

bool UserManager::ReadRegistry(const std::string& fileName) const {

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