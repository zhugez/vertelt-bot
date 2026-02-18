#include "vertel/runtime/shutdown.hpp"

#include <csignal>

namespace vertel::runtime {

std::atomic<bool> ShutdownSignal::stop_requested_{false};

void ShutdownSignal::Handle(int) { stop_requested_.store(true); }

void ShutdownSignal::Install() {
  std::signal(SIGINT, Handle);
  std::signal(SIGTERM, Handle);
}

bool ShutdownSignal::IsRequested() { return stop_requested_.load(); }

}  // namespace vertel::runtime
