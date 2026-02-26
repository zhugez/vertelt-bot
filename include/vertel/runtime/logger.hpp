#pragma once

#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>

namespace vertel::runtime {

enum class LogLevel { kInfo, kWarn, kError };

class Logger {
public:
  explicit Logger(std::ostream &out = std::cout);

  void Log(LogLevel level, std::string_view message,
           std::initializer_list<std::pair<std::string, std::string>> fields = {});

private:
  std::ostream &out_;
};

} // namespace vertel::runtime
