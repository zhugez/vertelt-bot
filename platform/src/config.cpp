#include "vertel/platform/config.hpp"

#include <cstdlib>
#include <sstream>
#include <string>

namespace vertel::platform {
namespace {

int ReadIntEnv(const char *key, int fallback) {
  if (const char *value = std::getenv(key); value != nullptr) {
    try {
      return std::stoi(value);
    } catch (...) {
      return fallback;
    }
  }
  return fallback;
}

std::unordered_set<std::int64_t> ReadAdminChatIds(const char *key) {
  std::unordered_set<std::int64_t> out;
  if (const char *value = std::getenv(key); value != nullptr) {
    std::stringstream ss(value);
    std::string token;
    while (std::getline(ss, token, ',')) {
      const auto start = token.find_first_not_of(" \t\n\r");
      if (start == std::string::npos) {
        continue;
      }
      const auto end = token.find_last_not_of(" \t\n\r");
      token = token.substr(start, end - start + 1);
      try {
        out.insert(std::stoll(token));
      } catch (...) {
        continue;
      }
    }
  }
  return out;
}

} // namespace

Config Config::FromEnv() {
  Config c;
  if (const char *token = std::getenv("TELEGRAM_BOT_TOKEN"); token != nullptr) {
    c.bot_token = token;
  }
  if (const char *inject = std::getenv("VERTEL_INJECT_SAMPLE_START"); inject != nullptr) {
    c.inject_sample_start = std::string(inject) != "0";
  }
  c.telegram_long_poll_timeout_seconds =
      ReadIntEnv("VERTEL_TELEGRAM_LONG_POLL_TIMEOUT_SECONDS", c.telegram_long_poll_timeout_seconds);
  c.telegram_request_timeout_seconds =
      ReadIntEnv("VERTEL_TELEGRAM_REQUEST_TIMEOUT_SECONDS", c.telegram_request_timeout_seconds);
  c.poll_max_attempts = ReadIntEnv("VERTEL_POLL_MAX_ATTEMPTS", c.poll_max_attempts);
  c.poll_initial_backoff_ms =
      ReadIntEnv("VERTEL_POLL_INITIAL_BACKOFF_MS", c.poll_initial_backoff_ms);
  c.loop_sleep_ms = ReadIntEnv("VERTEL_LOOP_SLEEP_MS", c.loop_sleep_ms);
  c.rate_limit_capacity = ReadIntEnv("VERTEL_RATE_LIMIT_CAPACITY", c.rate_limit_capacity);
  c.rate_limit_refill_tokens =
      ReadIntEnv("VERTEL_RATE_LIMIT_REFILL_TOKENS", c.rate_limit_refill_tokens);
  c.rate_limit_refill_seconds =
      ReadIntEnv("VERTEL_RATE_LIMIT_REFILL_SECONDS", c.rate_limit_refill_seconds);
  c.http_port = ReadIntEnv("VERTEL_HTTP_PORT", c.http_port);
  c.admin_chat_ids = ReadAdminChatIds("ADMIN_CHAT_IDS");
  return c;
}

} // namespace vertel::platform
