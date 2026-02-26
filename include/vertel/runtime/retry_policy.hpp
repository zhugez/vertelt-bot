#pragma once

#include <chrono>
#include <thread>

namespace vertel::runtime {

struct RetryPolicy {
  int max_attempts{3};
  std::chrono::milliseconds initial_backoff{100};

  template <typename Operation, typename IsRetryable>
  bool Execute(Operation &&operation, IsRetryable &&is_retryable) const {
    auto backoff = initial_backoff;
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
      if (operation()) {
        return true;
      }
      if (attempt == max_attempts || !is_retryable()) {
        return false;
      }
      std::this_thread::sleep_for(backoff);
      backoff *= 2;
    }
    return false;
  }
};

} // namespace vertel::runtime
