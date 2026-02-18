#pragma once

#include <cstdint>
#include <string>

namespace vertel::core {

struct Update {
  std::int64_t update_id{};
  std::int64_t chat_id{};
  std::string text;
};

struct OutgoingMessage {
  std::int64_t chat_id{};
  std::string text;
};

}  // namespace vertel::core
