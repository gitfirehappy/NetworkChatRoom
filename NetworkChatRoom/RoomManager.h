#pragma once
#include<unordered_map>
#include<vector>
#include"Room.h"

class RoomManager {
public:
    Room* CreateRoom(const std::string& name, int ownerID, const std::string& password = "");
    bool JoinRoom(int roomID, int userID, const std::string& password = "");
    bool LeaveRoom(int roomID, int userID); // 若房主退出则解散房间
    Room* GetRoom(int roomID) const;
    std::vector<Room*> GetAllRooms() const;

private:
    // 当前连接的Room管理
    std::unordered_map<int, Room> m_rooms;
    int m_nextRoomID = 1;
};