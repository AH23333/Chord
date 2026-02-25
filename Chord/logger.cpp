#include "logger.h"
#include <iostream>
#include <ctime>

using namespace std;

void Logger::init(const string &filename)
{
    logFile.open(filename, ios::app);
    if (logFile.is_open())
    {
        enabled = true;
        log("=== 程序启动 ===");
        time_t now = time(0);
        char *dt = ctime(&now);
        string timeStr = string(dt);
        while (!timeStr.empty() && (timeStr.back() == '\n' || timeStr.back() == '\r'))
            timeStr.pop_back();
        log("时间: " + timeStr);
    }
    else
    {
        cerr << "无法打开日志文件：" << filename << endl;
    }
}

void Logger::close()
{
    if (logFile.is_open())
    {
        log("=== 程序结束 ===");
        logFile.close();
    }
    enabled = false;
}

void Logger::log(const string &message)
{
    if (enabled && logFile.is_open())
    {
        time_t now = time(0);
        char *dt = ctime(&now);
        string timeStr = string(dt);
        while (!timeStr.empty() && (timeStr.back() == '\n' || timeStr.back() == '\r'))
            timeStr.pop_back();
        logFile << "[" << timeStr << "] " << message << endl;
        logFile.flush();
    }
}

void Logger::error(const string &message) { log("[ERROR] " + message); }
void Logger::warning(const string &message) { log("[WARNING] " + message); }
void Logger::info(const string &message) { log("[INFO] " + message); }
void Logger::debug(const string &message) { log("[DEBUG] " + message); }

Logger logger;