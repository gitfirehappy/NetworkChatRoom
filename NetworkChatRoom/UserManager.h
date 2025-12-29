#pragma once
#include<unordered_map>
#include<string>
#include<utility>
#include<iostream>
#include"User.h"

class UserManager {
public:
	bool ReadRegistry(const std::string& fileName) const;	// 读取注册表到缓存

	bool Register(const std::string& username, const std::string& password);
	bool Login(int id, const std::string& password);
	bool CancelAccount(int id, const std::string& password);
	bool HasRegisted(int id) const;
	bool CheckPassword(int id, const std::string& password);
	
	User* GetUser(const std::string& username);
	User* GetUser(int id);
	bool HasUser(const std::string& username) const;  
	bool HasUser(int id) const;  

	int GetNextUserID();
private:
	// 当前连接的Users管理
	std::unordered_map<std::string, User> m_usersByName;
	std::unordered_map<int, User> m_usersByID;
	
	// 记录所有用户信息的缓存表（从文件中读取）
	std::unordered_map<int,std::pair<std::string,std::string>> m_registryCache;

	int m_nextUserID = 1;	// TODO: 生成ID逻辑需要优化
};