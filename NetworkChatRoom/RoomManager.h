#pragma once
#include<unordered_map>
#include<vector>
#include"Room.h"

class RoomManager {
public:
    static RoomManager& GetInstance();
    
    Room* CreateRoom(const std::string& name, int ownerID, const std::string& password = "", int maxUsers = 10);
    bool JoinRoom(int roomID, int userID, const std::string& password = "");
    bool LeaveRoom(int roomID, int userID, bool isKick = false); // 若房主退出则解散房间
    bool DeleteRoom(int roomID); // 删除房间
    Room* GetRoom(int roomID);
    std::vector<Room*> GetAllRooms() const;
    bool IsRoomExists(int roomID) const; // 检查房间是否存在
    int GetNextRoomID(); // 获取下一个房间ID

private:
    // 当前连接的Room管理
    std::unordered_map<int, Room> m_rooms;
    int m_nextRoomID = 1;
};