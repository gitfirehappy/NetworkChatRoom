#include"Room.h"

Room::Room(int id, const std::string& name, int ownerID, const std::string& password, int maxUsers)
	: m_id(id), m_name(name), m_ownerID(ownerID), m_password(password), m_maxUsers(maxUsers) {

}

Room::~Room() {

}

int Room::GetID() const {
	return m_id;
}

std::string Room::GetName() const {
	return m_name;
}

int Room::GetOwnerID() const {
	return m_ownerID;
}

bool Room::HasPassword() const {
	return !m_password.empty();
}

bool Room::CheckPassword(const std::string& password) const {
	return m_password == password;
}

int Room::GetMaxUsers() const {
    return m_maxUsers;
}

void Room::AddUser(int userID) {
	if (!m_users.count(userID)) {
		m_users.insert(userID);
	}
}

void Room::RemoveUser(int userID) {
	if (m_users.count(userID)) {
		m_users.erase(userID);
	}
}

bool Room::ContainsUser(int userID) const {
	return m_users.count(userID);
}

size_t Room::GetUserCount() const {
	return m_users.size();
}

const std::unordered_set<int>& Room::GetUsers() const {
	return m_users; 
}

