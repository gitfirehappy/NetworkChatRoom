// 入口文件
#include"ChatServer.h"
#include"ChatClient.h"
#include <limits>

int main() {
    while (true) {
        std::cout << "Enter mode (0=Server, 1=Client, 2=Exit): ";
        int mode = -1;
        if (!(std::cin >> mode)) {
            std::cerr << "Invalid input. Please enter 0, 1, or 2.\n";
            std::cin.clear();
            std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
            continue;
        }
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

        try {
            if (mode == 0) {
                ChatServer server;
                server.Start();
                std::cout << "Press Enter to stop server...\n";
                std::cin.get();
                server.Stop();
                break;
            }
            else if (mode == 1) {
                ChatClient client;
                client.Start();
                break;
            }
            else if (mode == 2) {
                std::cout << "Goodbye!\n";
                break;
            }
            else {
                std::cerr << "Invalid mode. Please enter 0, 1, or 2.\n";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n\n";
        }
    }

    return 0;
}
