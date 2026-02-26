#pragma once

// MSVC: windows.h #defines SendMessage as SendMessageA/W — undo it.
#ifdef SendMessage
#undef SendMessage
#endif

#include <vector>

#include "vertel/core/message.hpp"

namespace vertel::core {

class TelegramGateway {
public:
  virtual ~TelegramGateway() = default;
  virtual std::vector<Update> PollUpdates() = 0;
  virtual void SendMessage(const OutgoingMessage &message) = 0;
};

} // namespace vertel::core
