#include "vertel/core/bot_service.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <utility>

namespace vertel::core {

std::optional<OutgoingMessage> StartCommandHandler::Handle(const Update& update) {
  if (update.text == "/start") {
    return OutgoingMessage{.chat_id = update.chat_id,
                           .text = "Welcome to VerTel Bot. Ready when you are."};
  }
  return std::nullopt;
}

std::optional<OutgoingMessage> HelpCommandHandler::Handle(const Update& update) {
  if (update.text == "/help") {
    return OutgoingMessage{
        .chat_id = update.chat_id,
        .text = "Available commands: /start, /help, /ping"};
  }
  return std::nullopt;
}

std::optional<OutgoingMessage> PingCommandHandler::Handle(const Update& update) {
  if (update.text == "/ping") {
    return OutgoingMessage{.chat_id = update.chat_id, .text = "pong"};
  }
  return std::nullopt;
}

CommandRouter::CommandRouter(std::vector<std::reference_wrapper<CommandHandler>> handlers)
    : handlers_(std::move(handlers)) {}

std::optional<OutgoingMessage> CommandRouter::Handle(const Update& update) {
  for (auto& handler : handlers_) {
    if (auto response = handler.get().Handle(update); response.has_value()) {
      return response;
    }
  }
  return std::nullopt;
}

TokenBucketRateLimiter::TokenBucketRateLimiter(int capacity, int refill_tokens,
                                               std::chrono::seconds refill_period)
    : capacity_(std::max(1, capacity)),
      refill_tokens_(std::max(1, refill_tokens)),
      refill_period_(std::max(std::chrono::seconds(1), refill_period)) {}

bool TokenBucketRateLimiter::Allow(std::int64_t chat_id) {
  const auto now = std::chrono::steady_clock::now();
  std::scoped_lock lock(mutex_);
  auto [it, _] = buckets_.emplace(chat_id, Bucket{});
  Bucket& bucket = it->second;
  if (bucket.last_refill.time_since_epoch().count() == 0) {
    bucket.tokens = static_cast<double>(capacity_);
    bucket.last_refill = now;
  }

  const auto elapsed = now - bucket.last_refill;
  const double periods =
      std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count() /
      static_cast<double>(refill_period_.count());
  if (periods > 0.0) {
    bucket.tokens =
        std::min(static_cast<double>(capacity_), bucket.tokens + periods * refill_tokens_);
    bucket.last_refill = now;
  }

  if (bucket.tokens < 1.0) {
    return false;
  }
  bucket.tokens -= 1.0;
  return true;
}

RateLimitedCommandHandler::RateLimitedCommandHandler(CommandHandler& inner,
                                                     TokenBucketRateLimiter& limiter,
                                                     std::string rejection_text,
                                                     runtime::MetricsRegistry* metrics)
    : inner_(inner),
      limiter_(limiter),
      rejection_text_(std::move(rejection_text)),
      metrics_(metrics) {}

std::optional<OutgoingMessage> RateLimitedCommandHandler::Handle(const Update& update) {
  if (!limiter_.Allow(update.chat_id)) {
    if (metrics_ != nullptr) {
      metrics_->IncrementRateLimitRejections();
    }
    return OutgoingMessage{.chat_id = update.chat_id, .text = rejection_text_};
  }
  return inner_.Handle(update);
}

AdminWhitelistCommandHandler::AdminWhitelistCommandHandler(
    CommandHandler& inner, std::unordered_set<std::int64_t> admin_chat_ids,
    std::string rejection_text)
    : inner_(inner),
      admin_chat_ids_(std::move(admin_chat_ids)),
      rejection_text_(std::move(rejection_text)) {}

std::optional<OutgoingMessage> AdminWhitelistCommandHandler::Handle(const Update& update) {
  if (admin_chat_ids_.empty() || admin_chat_ids_.contains(update.chat_id)) {
    return inner_.Handle(update);
  }
  return OutgoingMessage{.chat_id = update.chat_id, .text = rejection_text_};
}

BotService::BotService(TelegramGateway& gateway, CommandHandler& handler,
                       runtime::MetricsRegistry* metrics)
    : gateway_(gateway), handler_(handler), metrics_(metrics) {}

void BotService::ProcessOnce() {
  for (const auto& update : gateway_.PollUpdates()) {
    if (metrics_ != nullptr) {
      metrics_->IncrementUpdatesProcessed();
    }

    std::optional<OutgoingMessage> response;
    try {
      response = handler_.Handle(update);
    } catch (const std::exception&) {
      if (metrics_ != nullptr) {
        metrics_->IncrementHandlerFailures();
      }
      continue;
    }

    if (response.has_value()) {
      gateway_.SendMessage(*response);
      if (metrics_ != nullptr) {
        metrics_->IncrementMessagesSent();
      }
    }
  }
}

}  // namespace vertel::core
