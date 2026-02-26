#include "Logger.h"

Logger::Logger() : m_firstLog(true) {
    std::string logPath = GetExecutablePath() + "\\chat_server.log";
    m_logFile.open(logPath, std::ios::app);
    if (!m_logFile) {
        throw std::runtime_error("Failed to open log file: " + logPath);
    }
    m_logFile.seekp(0, std::ios::end);
    m_firstLog = (m_logFile.tellp() > 0);
}

Logger::~Logger() {
    m_logFile.close();
}

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

std::string Logger::GetExecutablePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string path(buffer);
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return path.substr(0, lastSlash);
    }
    return ".";
}

void Logger::Log(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);

    std::ostringstream oss;
    if (m_firstLog) {
        oss << "\n[" << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << "] " << message;
        m_firstLog = false;
    } else {
        oss << "[" << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << "] " << message;
    }

    m_logFile << oss.str() << std::endl;
}