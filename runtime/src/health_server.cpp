#include "vertel/runtime/health_server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sstream>

namespace vertel::runtime {
namespace {

std::string BuildHttpResponse(int status_code, const char* status_text, const std::string& body) {
  std::ostringstream response;
  response << "HTTP/1.1 " << status_code << ' ' << status_text << "\r\n";
  response << "Content-Type: text/plain; charset=utf-8\r\n";
  response << "Content-Length: " << body.size() << "\r\n";
  response << "Connection: close\r\n\r\n";
  response << body;
  return response.str();
}

std::string ExtractPath(const std::string& request) {
  const auto line_end = request.find("\r\n");
  const auto line = request.substr(0, line_end);
  const auto first_space = line.find(' ');
  if (first_space == std::string::npos) {
    return {};
  }
  const auto second_space = line.find(' ', first_space + 1);
  if (second_space == std::string::npos) {
    return {};
  }
  return line.substr(first_space + 1, second_space - first_space - 1);
}

}  // namespace

HealthServer::HealthServer(MetricsRegistry& metrics, int port)
    : metrics_(metrics), port_(port) {}

HealthServer::~HealthServer() { Stop(); }

void HealthServer::Start() {
  if (port_ <= 0) {
    return;
  }

  std::scoped_lock lock(mutex_);
  if (running_) {
    return;
  }

  running_ = true;
  thread_ = std::thread(&HealthServer::Run, this);
}

void HealthServer::Stop() {
  {
    std::scoped_lock lock(mutex_);
    if (!running_) {
      return;
    }
    running_ = false;
  }

  if (listen_fd_ >= 0) {
    shutdown(listen_fd_, SHUT_RDWR);
  }

  if (thread_.joinable()) {
    thread_.join();
  }
}

void HealthServer::Run() {
  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
    std::scoped_lock lock(mutex_);
    running_ = false;
    return;
  }

  constexpr int kEnable = 1;
  setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &kEnable, sizeof(kEnable));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(static_cast<uint16_t>(port_));

  if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
      listen(listen_fd_, 8) < 0) {
    close(listen_fd_);
    listen_fd_ = -1;
    std::scoped_lock lock(mutex_);
    running_ = false;
    return;
  }

  while (true) {
    {
      std::scoped_lock lock(mutex_);
      if (!running_) {
        break;
      }
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(listen_fd_, &read_fds);
    timeval timeout{};
    timeout.tv_sec = 1;

    const int ready = select(listen_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
    if (ready <= 0) {
      continue;
    }

    const int client_fd = accept(listen_fd_, nullptr, nullptr);
    if (client_fd < 0) {
      continue;
    }

    char buffer[2048];
    const ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
    std::string request;
    if (bytes_read > 0) {
      request.assign(buffer, static_cast<std::size_t>(bytes_read));
    }

    const std::string path = ExtractPath(request);

    std::string response;
    if (path == "/healthz") {
      response = BuildHttpResponse(200, "OK", "ok\n");
    } else if (path == "/metrics") {
      response = BuildHttpResponse(200, "OK", BuildMetricsBody(metrics_.Snapshot()));
    } else {
      response = BuildHttpResponse(404, "Not Found", "not found\n");
    }

    (void)send(client_fd, response.data(), response.size(), 0);
    close(client_fd);
  }

  close(listen_fd_);
  listen_fd_ = -1;
}

std::string HealthServer::BuildMetricsBody(const MetricsSnapshot& snapshot) {
  std::ostringstream out;
  out << "vertel_updates_processed_total " << snapshot.updates_processed << "\n";
  out << "vertel_messages_sent_total " << snapshot.messages_sent << "\n";
  out << "vertel_handler_failures_total " << snapshot.handler_failures << "\n";
  out << "vertel_rate_limit_rejections_total " << snapshot.rate_limit_rejections << "\n";
  return out.str();
}

}  // namespace vertel::runtime
