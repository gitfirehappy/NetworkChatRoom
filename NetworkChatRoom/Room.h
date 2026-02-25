#pragma once
#include<string>
#include<unordered_map>
#include<unordered_set>

/**
* 房间类
*/
class Room {
public:
	Room(int id, const std::string& name,int ownerID, const std::string& password = "", int maxUsers = 10);
	~Room();

	int GetID() const;
	std::string GetName() const;
    int GetOwnerID() const;
	bool HasPassword() const;
    bool CheckPassword(const std::string& password) const;
	int GetMaxUsers() const; // 获取最大用户数

	void AddUser(int userID);
	void RemoveUser(int userID);
	bool ContainsUser(int userID) const;
	size_t GetUserCount() const;
	const std::unordered_set<int>& GetUsers() const;

private:
	int m_id;		// 房间ID
	std::string m_name;		// 房间名称
	int m_ownerID;			// 房主ID
	std::string m_password;	// 房间密码
	int m_maxUsers;			// 最大用户数
	std::unordered_set<int> m_users;	// 房间用户集合
};
