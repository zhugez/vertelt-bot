# VerTel

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/std/the-standard)

A C++20 Telegram Bot library with built-in rate limiting, metrics, health checks, and retry logic.

## Quick example

```cpp
#include "vertel/adapters/telegram/telegram_client.hpp"
#include "vertel/core/bot_service.hpp"
#include "vertel/core/command_handler.hpp"
#include "vertel/runtime/shutdown.hpp"

int main() {
  using namespace vertel;

  // Connect to Telegram
  adapters::telegram::TelegramClient telegram("YOUR_BOT_TOKEN", 25, 35);

  // Register command handlers
  core::StartCommandHandler start_handler;  // responds to /start
  core::HelpCommandHandler  help_handler;   // responds to /help
  core::PingCommandHandler  ping_handler;   // responds to /ping
  core::CommandRouter router({start_handler, help_handler, ping_handler});

  // Wire up the bot
  core::BotService bot(telegram, router);

  // Poll until Ctrl-C
  runtime::ShutdownSignal::Install();
  while (!runtime::ShutdownSignal::IsRequested()) {
    bot.ProcessOnce();
  }
}
```

## Writing a custom command handler

Implement the `CommandHandler` interface to add your own commands:

```cpp
#include "vertel/core/command_handler.hpp"

class EchoCommandHandler final : public vertel::core::CommandHandler {
 public:
  std::optional<vertel::core::OutgoingMessage> Handle(
      const vertel::core::Update& update) override {
    if (update.text.starts_with("/echo ")) {
      return vertel::core::OutgoingMessage{
          .chat_id = update.chat_id,
          .text = update.text.substr(6)};
    }
    return std::nullopt;  // not my command, pass to next handler
  }
};
```

Then add it to the router:

```cpp
EchoCommandHandler echo_handler;
core::CommandRouter router({start_handler, echo_handler});
```

## Adding middleware

VerTel provides composable middleware wrappers. Each wraps a `CommandHandler` and adds behavior:

```cpp
#include "vertel/core/command_handler.hpp"
#include "vertel/runtime/metrics.hpp"

runtime::MetricsRegistry metrics;

// 1. Command router (innermost)
core::CommandRouter router({start_handler, help_handler, ping_handler});

// 2. Admin whitelist - only allow specific chat IDs (empty = allow all)
core::AdminWhitelistCommandHandler admin_guard(router, {123456789, 987654321});

// 3. Rate limiting - 5 messages per 10 seconds per chat
core::TokenBucketRateLimiter limiter(5, 5, std::chrono::seconds(10));
core::RateLimitedCommandHandler guarded(admin_guard, limiter,
                                        "Slow down!", &metrics);

// 4. Pass the outermost handler to BotService
core::BotService bot(telegram, guarded, &metrics);
```

The processing order is: **rate limit -> admin check -> command routing**.

## Observability

Add a health server for monitoring and container orchestration:

```cpp
#include "vertel/runtime/health_server.hpp"
#include "vertel/runtime/metrics.hpp"

runtime::MetricsRegistry metrics;
runtime::HealthServer health(metrics, 8080);
health.Start();

// ... run bot ...

health.Stop();
```

Endpoints:

| Endpoint | Response | Purpose |
|----------|----------|---------|
| `GET /healthz` | `200 ok` | Liveness probe |
| `GET /metrics` | Prometheus-style counters | Observability |

Exposed metrics:
- `vertel_updates_processed_total`
- `vertel_messages_sent_total`
- `vertel_handler_failures_total`
- `vertel_rate_limit_rejections_total`

## Structured logging

```cpp
#include "vertel/runtime/logger.hpp"

runtime::Logger logger;
logger.Log(runtime::LogLevel::kInfo, "bot_starting",
           {{"component", "app"}, {"version", "0.1.0"}});
```

Output (JSON to stdout):
```json
{"timestamp":"2025-01-15T10:30:00Z","level":"info","message":"bot_starting","component":"app","version":"0.1.0"}
```

## Retry with exponential backoff

```cpp
#include "vertel/runtime/retry_policy.hpp"

runtime::RetryPolicy retry{.max_attempts = 5,
                            .initial_backoff = std::chrono::milliseconds(250)};

bool ok = retry.Execute(
    [&] { bot.ProcessOnce(); return true; },
    [] { return true; });  // always retryable
```

Backoff doubles each attempt: 250ms -> 500ms -> 1s -> 2s.

## Building from source

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libcurl4-openssl-dev

# macOS
brew install cmake curl

# Fedora
sudo dnf install gcc-c++ cmake libcurl-devel
```

### Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `VERTEL_BUILD_TESTS` | `ON` | Build the test suite |
| `VERTEL_BUILD_EXAMPLES` | `ON` | Build `examples/basic_bot` |

libcurl is detected automatically. Without it, the library builds in sample-only mode (no live Telegram HTTP calls).

## Using VerTel as a library in your project

### Option 1: CMake install

```bash
# Build and install VerTel
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build --prefix /usr/local
```

In your project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_bot LANGUAGES CXX)

find_package(vertel CONFIG REQUIRED)

add_executable(my_bot main.cpp)
target_link_libraries(my_bot PRIVATE vertel::vertel)
```

### Option 2: CMake subdirectory (FetchContent)

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_bot LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(vertel
  GIT_REPOSITORY https://github.com/<your-org>/vertel-bot.git
  GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(vertel)

add_executable(my_bot main.cpp)
target_link_libraries(my_bot PRIVATE vertel)
```

### Option 3: Copy into your tree

Copy the `vertel-bot/` directory into your project and use `add_subdirectory()`:

```cmake
add_subdirectory(vendor/vertel-bot)
target_link_libraries(my_bot PRIVATE vertel)
```

## Running the reference bot

### Telegram mode

```bash
export TELEGRAM_BOT_TOKEN="<token-from-botfather>"
./build/vertel_basic_bot
```

### Sample mode (no Telegram needed)

```bash
VERTEL_INJECT_SAMPLE_START=1 ./build/vertel_basic_bot
```

Injects a fake `/start` update so you can verify the bot processes messages without a real Telegram connection.

### Docker

```bash
# With docker compose
echo "TELEGRAM_BOT_TOKEN=your-token" > .env
docker compose up --build

# Standalone
docker build -t vertel-bot .
docker run -e TELEGRAM_BOT_TOKEN=your-token vertel-bot
```

### systemd

```bash
sudo cp build/vertel_basic_bot /opt/vertel-bot/vertel_bot
sudo cp deploy/vertel-bot.service /etc/systemd/system/

# Set your token
echo "TELEGRAM_BOT_TOKEN=your-token" | sudo tee /etc/vertel-bot/vertel-bot.env

sudo systemctl enable --now vertel-bot
```

## Configuration

All configuration is via environment variables. Nothing is required when using VerTel as a library (you pass values directly in code). The reference bot reads these:

| Variable | Default | Description |
|----------|---------|-------------|
| `TELEGRAM_BOT_TOKEN` | *(required)* | Bot token from [@BotFather](https://t.me/BotFather) |
| `VERTEL_INJECT_SAMPLE_START` | `0` | Set `1` to inject a fake `/start` update (testing) |
| `VERTEL_TELEGRAM_LONG_POLL_TIMEOUT_SECONDS` | `25` | Telegram long-poll timeout |
| `VERTEL_TELEGRAM_REQUEST_TIMEOUT_SECONDS` | `35` | HTTP request timeout |
| `VERTEL_POLL_MAX_ATTEMPTS` | `5` | Max retry attempts per poll cycle |
| `VERTEL_POLL_INITIAL_BACKOFF_MS` | `250` | Initial retry backoff (doubles each attempt) |
| `VERTEL_LOOP_SLEEP_MS` | `50` | Sleep between poll cycles |
| `VERTEL_RATE_LIMIT_CAPACITY` | `5` | Token bucket capacity per chat |
| `VERTEL_RATE_LIMIT_REFILL_TOKENS` | `5` | Tokens refilled per period |
| `VERTEL_RATE_LIMIT_REFILL_SECONDS` | `10` | Refill period in seconds |
| `ADMIN_CHAT_IDS` | *(empty)* | Comma-separated allowed chat IDs (empty = all allowed) |
| `VERTEL_HTTP_PORT` | `8080` | Health/metrics server port (`<=0` to disable) |

## Architecture

```
your_main.cpp
    |
    v
+-------------------+
|       core        |   BotService, CommandHandler, CommandRouter,
|                   |   RateLimiter, AdminWhitelist
+--------+----------+
         |
    +----+----+
    |         |
+---v---+ +---v------+
|adapters| | runtime  |   Logger, RetryPolicy, Metrics,
|Telegram| | Health,  |   ShutdownSignal, HealthServer
|Client  | | Shutdown |
+---+---+ +----------+
    |
+---v------+
| platform |   Config::FromEnv()
+----------+
```

| Layer | Namespace | What it does |
|-------|-----------|-------------|
| **core** | `vertel::core` | Bot logic, command handling, middleware, message types |
| **adapters** | `vertel::adapters::telegram` | Telegram HTTP client (long polling + sendMessage) |
| **runtime** | `vertel::runtime` | Logging, metrics, health server, retry, graceful shutdown |
| **platform** | `vertel::platform` | Environment-based configuration parsing |

## API reference

### Core types

```cpp
namespace vertel::core {
  struct Update { int64_t update_id; int64_t chat_id; std::string text; };
  struct OutgoingMessage { int64_t chat_id; std::string text; };
}
```

### Key classes

| Class | Header | Purpose |
|-------|--------|---------|
| `BotService` | `vertel/core/bot_service.hpp` | Polls updates, dispatches to handler, sends replies |
| `CommandHandler` | `vertel/core/command_handler.hpp` | Abstract handler interface |
| `CommandRouter` | `vertel/core/command_handler.hpp` | Chains multiple handlers, returns first match |
| `TokenBucketRateLimiter` | `vertel/core/command_handler.hpp` | Per-chat rate limiting |
| `RateLimitedCommandHandler` | `vertel/core/command_handler.hpp` | Wraps a handler with rate limiting |
| `AdminWhitelistCommandHandler` | `vertel/core/command_handler.hpp` | Wraps a handler with chat ID whitelist |
| `TelegramGateway` | `vertel/core/telegram_gateway.hpp` | Abstract gateway (implement for testing) |
| `TelegramClient` | `vertel/adapters/telegram/telegram_client.hpp` | Production Telegram API client |
| `MetricsRegistry` | `vertel/runtime/metrics.hpp` | Atomic counters for observability |
| `HealthServer` | `vertel/runtime/health_server.hpp` | HTTP health/metrics endpoint |
| `Logger` | `vertel/runtime/logger.hpp` | Structured JSON logger |
| `RetryPolicy` | `vertel/runtime/retry_policy.hpp` | Exponential backoff retry |
| `ShutdownSignal` | `vertel/runtime/shutdown.hpp` | SIGINT/SIGTERM handler |
| `Config` | `vertel/platform/config.hpp` | `Config::FromEnv()` reads env vars |

## Testing with a fake gateway

The `TelegramGateway` interface lets you test without Telegram:

```cpp
#include "vertel/core/telegram_gateway.hpp"

class FakeGateway final : public vertel::core::TelegramGateway {
  std::vector<vertel::core::Update> updates_to_return;
  std::vector<vertel::core::OutgoingMessage> sent;

 public:
  void Enqueue(vertel::core::Update u) { updates_to_return.push_back(std::move(u)); }

  std::vector<vertel::core::Update> PollUpdates() override {
    auto result = std::move(updates_to_return);
    updates_to_return.clear();
    return result;
  }

  void SendMessage(const vertel::core::OutgoingMessage& msg) override {
    sent.push_back(msg);
  }

  const auto& Sent() const { return sent; }
};

// Usage in tests:
FakeGateway gateway;
gateway.Enqueue({.update_id = 1, .chat_id = 42, .text = "/ping"});

core::PingCommandHandler handler;
core::BotService bot(gateway, handler);
bot.ProcessOnce();

assert(gateway.Sent().size() == 1);
assert(gateway.Sent()[0].text == "pong");
```

## Contributing

- [CONTRIBUTING.md](CONTRIBUTING.md) - How to contribute
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) - Code of conduct
- [SECURITY.md](SECURITY.md) - Security policy
- [CHANGELOG.md](CHANGELOG.md) - Version history
- [OPERATIONS.md](OPERATIONS.md) - Runtime troubleshooting

## License

MIT License. See [LICENSE](LICENSE).
