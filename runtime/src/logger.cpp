#include "vertel/runtime/logger.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace vertel::runtime {
namespace {

std::string LevelToString(LogLevel level) {
  switch (level) {
    case LogLevel::kInfo:
      return "INFO";
    case LogLevel::kWarn:
      return "WARN";
    case LogLevel::kError:
      return "ERROR";
  }
  return "UNKNOWN";
}

std::string UtcTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto tt = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &tt);
#else
  gmtime_r(&tt, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

std::string Escape(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  for (char c : in) {
    if (c == '"' || c == '\\') out.push_back('\\');
    out.push_back(c);
  }
  return out;
}

}  // namespace

Logger::Logger(std::ostream& out) : out_(out) {}

void Logger::Log(
    LogLevel level, std::string_view message,
    std::initializer_list<std::pair<std::string, std::string>> fields) {
  out_ << "{\"ts\":\"" << UtcTimestamp() << "\",\"level\":\""
       << LevelToString(level) << "\",\"msg\":\"" << Escape(message)
       << "\"";
  for (const auto& [k, v] : fields) {
    out_ << ",\"" << Escape(k) << "\":\"" << Escape(v) << "\"";
  }
  out_ << "}" << std::endl;
}

}  // namespace vertel::runtime
