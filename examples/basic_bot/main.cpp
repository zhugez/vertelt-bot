#include <chrono>
#include <exception>
#include <thread>

#include "vertel/adapters/telegram/telegram_client.hpp"
#include "vertel/core/bot_service.hpp"
#include "vertel/platform/config.hpp"
#include "vertel/runtime/health_server.hpp"
#include "vertel/runtime/logger.hpp"
#include "vertel/runtime/metrics.hpp"
#include "vertel/runtime/retry_policy.hpp"
#include "vertel/runtime/shutdown.hpp"

int main() {
  using namespace vertel;

  const auto config = platform::Config::FromEnv();
  runtime::Logger logger;
  runtime::RetryPolicy retry_policy{
      .max_attempts = config.poll_max_attempts,
      .initial_backoff = std::chrono::milliseconds(config.poll_initial_backoff_ms)};
  runtime::ShutdownSignal::Install();
  runtime::MetricsRegistry metrics;
  runtime::HealthServer health_server(metrics, config.http_port);
  health_server.Start();

  adapters::telegram::TelegramClient telegram(
      config.inject_sample_start
          ? adapters::telegram::TelegramClient(/*inject_sample_update=*/true)
          : adapters::telegram::TelegramClient(config.bot_token,
                                               config.telegram_long_poll_timeout_seconds,
                                               config.telegram_request_timeout_seconds));

  core::StartCommandHandler start_handler;
  core::HelpCommandHandler help_handler;
  core::PingCommandHandler ping_handler;
  core::CommandRouter router({start_handler, help_handler, ping_handler});
  core::AdminWhitelistCommandHandler admin_guard(router, config.admin_chat_ids);
  core::TokenBucketRateLimiter limiter(
      config.rate_limit_capacity, config.rate_limit_refill_tokens,
      std::chrono::seconds(config.rate_limit_refill_seconds));
  core::RateLimitedCommandHandler guarded_router(admin_guard, limiter,
                                                 "Rate limit exceeded. Please slow down.",
                                                 &metrics);
  core::BotService bot(telegram, guarded_router, &metrics);

  logger.Log(runtime::LogLevel::kInfo, "bot_starting",
             {{"component", "app"}, {"has_token", config.bot_token.empty() ? "false" : "true"}});

  while (!runtime::ShutdownSignal::IsRequested()) {
    const bool processed = retry_policy.Execute(
        [&] {
          try {
            bot.ProcessOnce();
            return true;
          } catch (const std::exception& ex) {
            logger.Log(runtime::LogLevel::kWarn, "poll_iteration_failed",
                       {{"component", "app"}, {"error", ex.what()}});
            return false;
          }
        },
        [] { return true; });

    if (!processed) {
      logger.Log(runtime::LogLevel::kError, "poll_iteration_exhausted",
                 {{"component", "app"}});
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(config.loop_sleep_ms));

    if (config.inject_sample_start) {
      break;
    }
  }

  logger.Log(runtime::LogLevel::kInfo, "bot_stopped", {{"component", "app"}});
  health_server.Stop();
  return 0;
}
