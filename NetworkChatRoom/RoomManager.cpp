#include "RoomManager.h"

RoomManager& RoomManager::GetInstance() {
    static RoomManager instance;
    return instance;
}

Room* RoomManager::CreateRoom(const std::string& name, int ownerID, const std::string& password, int maxUsers) {
    int roomID = m_nextRoomID++;
    Room room(roomID, name, ownerID, password, maxUsers);
    room.AddUser(ownerID);
    auto inserted = m_rooms.emplace(roomID, room);
    if (!inserted.second) {
        return nullptr;
    }
    return &inserted.first->second;
}

bool RoomManager::JoinRoom(int roomID, int userID, const std::string& password) {
    Room* room = GetRoom(roomID);
    if(!room) return false;

    if(room->HasPassword() && !room->CheckPassword(password)) return false;
    
    // 检查房间是否已满
    if(room->GetUserCount() >= room->GetMaxUsers()) {
        return false;
    }

    room->AddUser(userID);
    return true;
}

bool RoomManager::LeaveRoom(int roomID, int userID, bool isKick /*= false*/) {
    Room* room = GetRoom(roomID);
    if (!room) return false;

    bool isOwnerLeaving = (room->GetOwnerID() == userID);
    room->RemoveUser(userID);
    
    // 如果是踢出用户（isKick=true），不解散房间，除非房间没有人了
    if (isKick) {
        if (room->GetUserCount() == 0) {
            DeleteRoom(roomID);
        }
    } else {
        // 如果是房主主动离开且房间还有其他用户，则解散房间
        if (isOwnerLeaving) {
            if (room->GetUserCount() > 1) {
                DeleteRoom(roomID);
            } else {
                DeleteRoom(roomID);
            }
        } else if (room->GetUserCount() == 0) {
            DeleteRoom(roomID);
        }
    }
    
    return true;
}

bool RoomManager::DeleteRoom(int roomID) {
    auto it = m_rooms.find(roomID);
    if (it != m_rooms.end()) {
        m_rooms.erase(it);
        return true;
    }
    return false;
}

Room* RoomManager::GetRoom(int roomID) {
    auto it = m_rooms.find(roomID);
    if (it != m_rooms.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<Room*> RoomManager::GetAllRooms() const {
    std::vector<Room*> rooms;
    for (auto& pair : m_rooms) {
        // 创建临时指针并将其添加到向量中
        Room* roomPtr = const_cast<Room*>(&pair.second);
        rooms.emplace_back(roomPtr);
    }
    return rooms;
}

bool RoomManager::IsRoomExists(int roomID) const {
    return m_rooms.find(roomID) != m_rooms.end();
}

int RoomManager::GetNextRoomID() {
    return m_nextRoomID;
}