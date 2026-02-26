#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "vertel/core/message.hpp"
#include "vertel/runtime/metrics.hpp"

namespace vertel::core {

class CommandHandler {
public:
  virtual ~CommandHandler() = default;
  virtual std::optional<OutgoingMessage> Handle(const Update &update) = 0;
};

class StartCommandHandler final : public CommandHandler {
public:
  std::optional<OutgoingMessage> Handle(const Update &update) override;
};

class HelpCommandHandler final : public CommandHandler {
public:
  std::optional<OutgoingMessage> Handle(const Update &update) override;
};

class PingCommandHandler final : public CommandHandler {
public:
  std::optional<OutgoingMessage> Handle(const Update &update) override;
};

class CommandRouter final : public CommandHandler {
public:
  explicit CommandRouter(std::vector<std::reference_wrapper<CommandHandler>> handlers);

  std::optional<OutgoingMessage> Handle(const Update &update) override;

private:
  std::vector<std::reference_wrapper<CommandHandler>> handlers_;
};

class TokenBucketRateLimiter {
public:
  TokenBucketRateLimiter(int capacity, int refill_tokens,
                         std::chrono::seconds refill_period = std::chrono::seconds(1));

  bool Allow(std::int64_t chat_id);

private:
  struct Bucket {
    double tokens{0.0};
    std::chrono::steady_clock::time_point last_refill;
  };

  int capacity_;
  int refill_tokens_;
  std::chrono::seconds refill_period_;
  std::mutex mutex_;
  std::unordered_map<std::int64_t, Bucket> buckets_;
};

class RateLimitedCommandHandler final : public CommandHandler {
public:
  RateLimitedCommandHandler(CommandHandler &inner, TokenBucketRateLimiter &limiter,
                            std::string rejection_text = "Rate limit exceeded. Please slow down.",
                            runtime::MetricsRegistry *metrics = nullptr);

  std::optional<OutgoingMessage> Handle(const Update &update) override;

private:
  CommandHandler &inner_;
  TokenBucketRateLimiter &limiter_;
  std::string rejection_text_;
  runtime::MetricsRegistry *metrics_{nullptr};
};

class AdminWhitelistCommandHandler final : public CommandHandler {
public:
  AdminWhitelistCommandHandler(CommandHandler &inner,
                               std::unordered_set<std::int64_t> admin_chat_ids,
                               std::string rejection_text = "Unauthorized.");

  std::optional<OutgoingMessage> Handle(const Update &update) override;

private:
  CommandHandler &inner_;
  std::unordered_set<std::int64_t> admin_chat_ids_;
  std::string rejection_text_;
};

} // namespace vertel::core
