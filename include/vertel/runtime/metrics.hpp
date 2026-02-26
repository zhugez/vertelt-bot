#pragma once

#include <atomic>
#include <cstdint>

namespace vertel::runtime {

struct MetricsSnapshot {
  std::uint64_t updates_processed{0};
  std::uint64_t messages_sent{0};
  std::uint64_t handler_failures{0};
  std::uint64_t rate_limit_rejections{0};
};

class MetricsRegistry {
public:
  void IncrementUpdatesProcessed() { updates_processed_.fetch_add(1, std::memory_order_relaxed); }
  void IncrementMessagesSent() { messages_sent_.fetch_add(1, std::memory_order_relaxed); }
  void IncrementHandlerFailures() { handler_failures_.fetch_add(1, std::memory_order_relaxed); }
  void IncrementRateLimitRejections() {
    rate_limit_rejections_.fetch_add(1, std::memory_order_relaxed);
  }

  MetricsSnapshot Snapshot() const {
    return MetricsSnapshot{.updates_processed = updates_processed_.load(std::memory_order_relaxed),
                           .messages_sent = messages_sent_.load(std::memory_order_relaxed),
                           .handler_failures = handler_failures_.load(std::memory_order_relaxed),
                           .rate_limit_rejections =
                               rate_limit_rejections_.load(std::memory_order_relaxed)};
  }

private:
  std::atomic<std::uint64_t> updates_processed_{0};
  std::atomic<std::uint64_t> messages_sent_{0};
  std::atomic<std::uint64_t> handler_failures_{0};
  std::atomic<std::uint64_t> rate_limit_rejections_{0};
};

} // namespace vertel::runtime
