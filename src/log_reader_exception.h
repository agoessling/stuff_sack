#pragma once

#include <stdexcept>

class LogParseException : public std::runtime_error {
 public:
  LogParseException(const std::string& what_arg)
      : std::runtime_error("LogParseException: " + what_arg) {}
};
