#pragma once

#include "vertel/core/command_handler.hpp"
#include "vertel/core/telegram_gateway.hpp"
#include "vertel/runtime/metrics.hpp"

namespace vertel::core {

class BotService {
 public:
  BotService(TelegramGateway& gateway, CommandHandler& handler,
             runtime::MetricsRegistry* metrics = nullptr);

  void ProcessOnce();

 private:
  TelegramGateway& gateway_;
  CommandHandler& handler_;
  runtime::MetricsRegistry* metrics_;
};

}  // namespace vertel::core
