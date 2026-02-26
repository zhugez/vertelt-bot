#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "vertel/core/telegram_gateway.hpp"

// MSVC: windows.h (transitively via curl/curl.h) #defines SendMessage as
// SendMessageA/W. Undo it so our method name compiles correctly.
#ifdef SendMessage
#undef SendMessage
#endif

namespace vertel::adapters::telegram {

class TelegramClient final : public vertel::core::TelegramGateway {
public:
  explicit TelegramClient(bool inject_sample_update);
  TelegramClient(std::string bot_token, int long_poll_timeout_seconds, int request_timeout_seconds);

  std::vector<vertel::core::Update> PollUpdates() override;
  void SendMessage(const vertel::core::OutgoingMessage &message) override;

  const std::vector<vertel::core::OutgoingMessage> &SentMessages() const;

private:
  struct HttpResponse {
    long status_code{0};
    std::string body;
  };

  HttpResponse PostForm(const std::string &endpoint, const std::string &form_body) const;
  std::vector<vertel::core::Update> ParseUpdates(const std::string &json) const;
  static std::string UrlEncode(const std::string &value);

  bool inject_sample_update_{false};
  bool sample_emitted_{false};
  std::string bot_token_;
  int long_poll_timeout_seconds_{25};
  int request_timeout_seconds_{35};
  std::int64_t next_update_offset_{0};
  std::vector<vertel::core::OutgoingMessage> sent_messages_;
};

} // namespace vertel::adapters::telegram
