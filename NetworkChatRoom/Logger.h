#pragma once
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <windows.h>

class Logger {
public:
    static Logger& GetInstance();
    void Log(const std::string& message);

private:
    Logger();
    ~Logger();
    std::string GetExecutablePath();
    std::ofstream m_logFile;
    std::mutex m_mutex;
};