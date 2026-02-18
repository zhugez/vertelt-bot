#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

namespace vertel::platform {

struct Config {
  std::string bot_token;
  bool inject_sample_start{false};
  int telegram_long_poll_timeout_seconds{25};
  int telegram_request_timeout_seconds{35};
  int poll_max_attempts{5};
  int poll_initial_backoff_ms{250};
  int loop_sleep_ms{50};
  int rate_limit_capacity{5};
  int rate_limit_refill_tokens{5};
  int rate_limit_refill_seconds{10};
  int http_port{8080};
  std::unordered_set<std::int64_t> admin_chat_ids;

  static Config FromEnv();
};

}  // namespace vertel::platform
