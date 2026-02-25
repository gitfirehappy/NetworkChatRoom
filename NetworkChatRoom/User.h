#pragma once
#include<string>
#include <WinSock2.h>
#include <WS2tcpip.h>

// 用户状态
enum class UserStatus {
	IN_LOBBY,	// 在大厅
	IN_ROOM,	// 在房间
};

/**
* 用户类
*/
class User {
public:
	explicit User(int id, const std::string& username, SOCKET socket)
		: m_id(id), m_username(username), m_socket(socket) {
	
	};

	~User() {};

	int GetID() const { return m_id; };
	std::string GetUsername() const { return m_username; };
	SOCKET GetSocket() const { return m_socket; };
	UserStatus GetUserStatus() const { return m_status; };
	int GetRoomID() const { return m_currentRoomID; };
	void SetStatus(UserStatus status) { m_status = status; };
	void SetRoom(int roomID) { m_currentRoomID = roomID; };
	void SetPassword(const std::string& password) { m_password = password; };

private:
	int m_id;						// 用户ID
	std::string m_username;			// 用户名
	std::string m_password;			// 密码
	SOCKET m_socket;				// 用户 socket
	UserStatus m_status;			// 用户状态
	int m_currentRoomID;			// 当前所在房间ID
};
