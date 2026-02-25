#include "Logger.h"

Logger::Logger() {
    m_logFile.open("chat_server.log", std::ios::app);
    if (!m_logFile) {
        throw std::runtime_error("Failed to open log file");
    }
}

Logger::~Logger() {
    m_logFile.close();
}

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

void Logger::Log(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);

    std::ostringstream oss;
    oss << "[" << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << "] " << message;

    m_logFile << oss.str() << std::endl;
}