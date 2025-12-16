// 入口文件
#include"ChatServer.h"
#include"ChatClient.h"

int main() {
    // 选择运行模式: 0=服务器, 1=客户端
    int mode;
    std::cout << "Enter mode (0=Server, 1=Client): ";
    std::cin >> mode;
    std::cin.ignore();

    try {
        if (mode == 0) {
            ChatServer server;
            server.Start();
            std::cout << "Press Enter to stop server...\n";
            std::cin.get();
            server.Stop();
        }
        else if (mode == 1) {
            ChatClient client;
            client.Start();
        }
        else {
            std::cerr << "Invalid mode\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}