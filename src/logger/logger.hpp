// logger.hpp, shoutout ChatGPT for the basic logger.
#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>

namespace fs = std::filesystem;

class SimpleLogger {
public:
    SimpleLogger(const fs::path& logPath);
    void log(const std::string& message, const std::string& logType);
    ~SimpleLogger();

private:
    fs::path logPath;
    std::ofstream logFile;
};