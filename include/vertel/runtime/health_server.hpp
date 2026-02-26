#pragma once

#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "vertel/runtime/metrics.hpp"

namespace vertel::runtime {

class HealthServer {
public:
  HealthServer(MetricsRegistry &metrics, int port);
  ~HealthServer();

  HealthServer(const HealthServer &) = delete;
  HealthServer &operator=(const HealthServer &) = delete;

  void Start();
  void Stop();

  int port() const { return port_; }

private:
  void Run();
  static std::string BuildMetricsBody(const MetricsSnapshot &snapshot);

  MetricsRegistry &metrics_;
  int port_;
  bool running_{false};
#ifdef _WIN32
  SOCKET listen_fd_{INVALID_SOCKET};
#else
  int listen_fd_{-1};
#endif
  std::thread thread_;
  std::mutex mutex_;
};

} // namespace vertel::runtime
