#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>

class Logger
{
private:
    std::ofstream logFile;
    bool enabled;

public:
    Logger() : enabled(false) {}
    void init(const std::string &filename);
    void close();
    void log(const std::string &message);
    void error(const std::string &message);
    void warning(const std::string &message);
    void info(const std::string &message);
    void debug(const std::string &message);
};

extern Logger logger;

#endif // LOGGER_H