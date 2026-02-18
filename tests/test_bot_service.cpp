#include <cassert>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

#include "vertel/adapters/telegram/telegram_client.hpp"
#include "vertel/core/bot_service.hpp"
#include "vertel/runtime/metrics.hpp"

namespace {

class FakeGateway final : public vertel::core::TelegramGateway {
 public:
  explicit FakeGateway(std::vector<vertel::core::Update> updates)
      : updates_(std::move(updates)) {}

  std::vector<vertel::core::Update> PollUpdates() override {
    auto out = updates_;
    updates_.clear();
    return out;
  }

  void SendMessage(const vertel::core::OutgoingMessage& message) override {
    sent_.push_back(message);
  }

  const std::vector<vertel::core::OutgoingMessage>& Sent() const { return sent_; }

 private:
  std::vector<vertel::core::Update> updates_;
  std::vector<vertel::core::OutgoingMessage> sent_;
};

class ThrowingHandler final : public vertel::core::CommandHandler {
 public:
  std::optional<vertel::core::OutgoingMessage> Handle(const vertel::core::Update& update) override {
    if (update.text == "/boom") {
      throw std::runtime_error("simulated handler failure");
    }
    return vertel::core::OutgoingMessage{.chat_id = update.chat_id, .text = "ok"};
  }
};

void TestStartCommandWithAdapterSample() {
  vertel::adapters::telegram::TelegramClient telegram(/*inject_sample_update=*/true);
  vertel::core::StartCommandHandler start_handler;
  vertel::core::BotService bot(telegram, start_handler);

  bot.ProcessOnce();

  const auto& sent = telegram.SentMessages();
  assert(sent.size() == 1);
  assert(sent[0].chat_id == 1001);
  assert(sent[0].text.find("Welcome") != std::string::npos);
}

void TestRouterHandlesHelpAndPing() {
  FakeGateway gateway({
      {.update_id = 1, .chat_id = 11, .text = "/help"},
      {.update_id = 2, .chat_id = 12, .text = "/ping"},
      {.update_id = 3, .chat_id = 13, .text = "/unknown"},
  });

  vertel::core::StartCommandHandler start_handler;
  vertel::core::HelpCommandHandler help_handler;
  vertel::core::PingCommandHandler ping_handler;
  vertel::core::CommandRouter router({start_handler, help_handler, ping_handler});
  vertel::core::BotService bot(gateway, router);

  bot.ProcessOnce();

  const auto& sent = gateway.Sent();
  assert(sent.size() == 2);
  assert(sent[0].chat_id == 11);
  assert(sent[0].text.find("/start") != std::string::npos);
  assert(sent[1].chat_id == 12);
  assert(sent[1].text == "pong");
}

void TestRateLimiterBlocksBurstPerChatAndIncrementsMetric() {
  FakeGateway gateway({
      {.update_id = 1, .chat_id = 77, .text = "/ping"},
      {.update_id = 2, .chat_id = 77, .text = "/ping"},
      {.update_id = 3, .chat_id = 77, .text = "/ping"},
  });

  vertel::runtime::MetricsRegistry metrics;
  vertel::core::PingCommandHandler ping_handler;
  vertel::core::TokenBucketRateLimiter limiter(
      /*capacity=*/2, /*refill_tokens=*/1, std::chrono::seconds(60));
  vertel::core::RateLimitedCommandHandler limited(ping_handler, limiter,
                                                  "Rate limit exceeded. Please slow down.",
                                                  &metrics);
  vertel::core::BotService bot(gateway, limited, &metrics);

  bot.ProcessOnce();

  const auto& sent = gateway.Sent();
  assert(sent.size() == 3);
  assert(sent[0].text == "pong");
  assert(sent[1].text == "pong");
  assert(sent[2].text.find("Rate limit exceeded") != std::string::npos);

  const auto snapshot = metrics.Snapshot();
  assert(snapshot.rate_limit_rejections == 1);
  assert(snapshot.updates_processed == 3);
  assert(snapshot.messages_sent == 3);
}

void TestAdminWhitelistBlocksNonAdmin() {
  FakeGateway gateway({
      {.update_id = 1, .chat_id = 200, .text = "/ping"},
      {.update_id = 2, .chat_id = 100, .text = "/ping"},
  });

  vertel::core::PingCommandHandler ping_handler;
  vertel::core::AdminWhitelistCommandHandler admin_guard(
      ping_handler, std::unordered_set<std::int64_t>{100});
  vertel::core::BotService bot(gateway, admin_guard);

  bot.ProcessOnce();

  const auto& sent = gateway.Sent();
  assert(sent.size() == 2);
  assert(sent[0].chat_id == 200);
  assert(sent[0].text == "Unauthorized.");
  assert(sent[1].chat_id == 100);
  assert(sent[1].text == "pong");
}

void TestHandlerFailuresAreCountedAndProcessingContinues() {
  FakeGateway gateway({
      {.update_id = 1, .chat_id = 101, .text = "/boom"},
      {.update_id = 2, .chat_id = 101, .text = "/ping"},
  });

  vertel::runtime::MetricsRegistry metrics;
  ThrowingHandler handler;
  vertel::core::BotService bot(gateway, handler, &metrics);

  bot.ProcessOnce();

  const auto& sent = gateway.Sent();
  assert(sent.size() == 1);
  assert(sent[0].text == "ok");

  const auto snapshot = metrics.Snapshot();
  assert(snapshot.updates_processed == 2);
  assert(snapshot.handler_failures == 1);
  assert(snapshot.messages_sent == 1);
}

}  // namespace

int main() {
  TestStartCommandWithAdapterSample();
  TestRouterHandlesHelpAndPing();
  TestRateLimiterBlocksBurstPerChatAndIncrementsMetric();
  TestAdminWhitelistBlocksNonAdmin();
  TestHandlerFailuresAreCountedAndProcessingContinues();
  return 0;
}
