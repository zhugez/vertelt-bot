#include "vertel/runtime/health_server.hpp"

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
// winsock2.h already included via the header
#  pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
constexpr socket_t kInvalidSocket = INVALID_SOCKET;
inline int close_socket(socket_t s) { return closesocket(s); }
inline void shutdown_socket(socket_t s) { shutdown(s, SD_BOTH); }
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <unistd.h>
using socket_t = int;
constexpr socket_t kInvalidSocket = -1;
inline int close_socket(socket_t s) { return close(s); }
inline void shutdown_socket(socket_t s) { shutdown(s, SHUT_RDWR); }
#endif

#include <sstream>

namespace vertel::runtime {
namespace {

#ifdef _WIN32
struct WinsockInit {
  WinsockInit() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
  }
  ~WinsockInit() { WSACleanup(); }
};
static WinsockInit winsock_init_;
#endif

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

  if (listen_fd_ != kInvalidSocket) {
    shutdown_socket(listen_fd_);
  }

  if (thread_.joinable()) {
    thread_.join();
  }
}

void HealthServer::Run() {
  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ == kInvalidSocket) {
    std::scoped_lock lock(mutex_);
    running_ = false;
    return;
  }

#ifdef _WIN32
  const char kEnable = 1;
  setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &kEnable, sizeof(kEnable));
#else
  constexpr int kEnable = 1;
  setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &kEnable, sizeof(kEnable));
#endif

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(static_cast<uint16_t>(port_));

  if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
      listen(listen_fd_, 8) < 0) {
    close_socket(listen_fd_);
    listen_fd_ = kInvalidSocket;
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

#ifdef _WIN32
    // Windows ignores the first param of select; pass 0
    const int ready = select(0, &read_fds, nullptr, nullptr, &timeout);
#else
    const int ready = select(listen_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
#endif
    if (ready <= 0) {
      continue;
    }

    const socket_t client_fd = accept(listen_fd_, nullptr, nullptr);
    if (client_fd == kInvalidSocket) {
      continue;
    }

    char buffer[2048];
    const int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
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

    (void)send(client_fd, response.data(), static_cast<int>(response.size()), 0);
    close_socket(client_fd);
  }

  close_socket(listen_fd_);
  listen_fd_ = kInvalidSocket;
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
