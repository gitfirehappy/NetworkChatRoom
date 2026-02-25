#pragma once
#include<unordered_map>
#include<string>
#include<utility>
#include<iostream>
#include"User.h"
#include<vector>

class UserManager {
public:
	static UserManager& GetInstance();
	
	explicit UserManager(const std::string& registryFile = "registry.txt");
	~UserManager(); // 添加析构函数来保存数据

	bool ReadRegistry(const std::string& fileName);	// 读取注册表到缓存
	bool SaveRegistry(const std::string& fileName); // 添加保存功能
	bool Register(const std::string& username, const std::string& password);
	bool Login(int id, const std::string& password, SOCKET socket);
	bool CancelAccount(int id, const std::string& password);
	bool HasRegisted(int id) const;
	bool CheckPassword(int id, const std::string& password);
	
	User* GetUser(const std::string& username);
	User* GetUser(int id);
	bool HasUser(const std::string& username) const;  
	bool HasUser(int id) const;  

	int GetNextUserID();
	
	// 添加用户到当前连接列表
	void AddConnectedUser(int id, const std::string& username, SOCKET socket);
	// 移除连接用户
	void RemoveConnectedUser(int id);
	// 获取当前在线用户列表
	std::vector<User*> GetOnlineUsers() const;

private:
	// 当前连接的Users管理
	std::unordered_map<std::string, User> m_usersByName;
	std::unordered_map<int, User> m_usersByID;
	
	// 记录所有用户信息的缓存表（从文件中读取）
	std::unordered_map<int,std::pair<std::string,std::string>> m_registryCache;

	int m_nextUserID = 1;
	std::string m_registryFile;
};