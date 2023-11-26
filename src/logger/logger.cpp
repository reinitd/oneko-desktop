// logger.cpp, shoutout ChatGPT for the basic logger.
#include "logger.hpp"

SimpleLogger::SimpleLogger(const fs::path& logPath) : logPath(logPath) {
    logFile.open(logPath, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Error: Unable to open log file." << std::endl;
    }
}

void SimpleLogger::log(const std::string& message, const std::string& logType) {
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&nowTime));

        logFile << "[" << buffer << "] [" << logType << "] " << message << std::endl;
    } else {
        std::cerr << "Error: Log file is not open." << std::endl;
    }
}

SimpleLogger::~SimpleLogger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}