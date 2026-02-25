#pragma once
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <chrono>

class Logger {
public:
    static Logger& GetInstance();
    void Log(const std::string& message);

private:
    Logger();
    ~Logger();
    std::ofstream m_logFile;
    std::mutex m_mutex;
};