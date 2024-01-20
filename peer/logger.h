#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <vector>

enum class LogLevel {
  None,
  Error,
  Warning,
  Info,
  Debug,
};

class Logger {
public:
  Logger(LogLevel level) : currentLevel(level) {}

  void log(const std::string &message, LogLevel level) {
    if (level <= currentLevel) {
      std::cout << levelStr[int(currentLevel) - 1] << ": " << message
                << std::endl;
    }
  }

private:
  LogLevel currentLevel;

  std::vector<std::string> levelStr = {"[Error]", "[Warning]", "[Info]",
                                       "[Debug]"};
};

#endif
