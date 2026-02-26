#pragma once

#include <atomic>

namespace vertel::runtime {

class ShutdownSignal {
public:
  static void Install();
  static bool IsRequested();

private:
  static void Handle(int signal_number);
  static std::atomic<bool> stop_requested_;
};

} // namespace vertel::runtime
