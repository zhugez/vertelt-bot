<p align="center">
  <img src="docs/banner.png?v=3" alt="VerTel — C++20 Telegram Bot Framework" width="480">
</p>

<h1 align="center">VerTel</h1>

<p align="center">
  <strong>A modern C++20 Telegram Bot framework with built-in rate limiting, metrics, health checks, and retry logic.</strong>
</p>

<p align="center">
  <a href="LICENSE"><img alt="License: MIT" src="https://img.shields.io/badge/License-MIT-yellow.svg"></a>
  <a href="https://isocpp.org/std/the-standard"><img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-blue.svg"></a>
  <a href="https://cmake.org"><img alt="CMake" src="https://img.shields.io/badge/Build-CMake-064F8C.svg?logo=cmake"></a>
  <a href="https://www.docker.com"><img alt="Docker" src="https://img.shields.io/badge/Docker-Ready-2496ED.svg?logo=docker&logoColor=white"></a>
</p>

---

## ✨ Highlights

- **Zero-config quickstart** — just supply a bot token and go
- **Composable middleware** — rate limiting, admin whitelists, and custom handlers snap together
- **Production-ready observability** — Prometheus metrics, `/healthz` endpoint, structured JSON logging
- **Retry with exponential backoff** — built-in resilience for flaky networks
- **Testable by design** — swap in a fake gateway for fully offline unit tests
- **Deploy anywhere** — Docker, systemd, or bare metal

---

## 🚀 Quick Start

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

---

## 🔧 Writing a Custom Command Handler

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

---

## 🧩 Adding Middleware

VerTel provides composable middleware wrappers. Each wraps a `CommandHandler` and adds behavior:

```cpp
#include "vertel/core/command_handler.hpp"
#include "vertel/runtime/metrics.hpp"

runtime::MetricsRegistry metrics;

// 1. Command router (innermost)
core::CommandRouter router({start_handler, help_handler, ping_handler});

// 2. Admin whitelist — only allow specific chat IDs (empty = allow all)
core::AdminWhitelistCommandHandler admin_guard(router, {123456789, 987654321});

// 3. Rate limiting — 5 messages per 10 seconds per chat
core::TokenBucketRateLimiter limiter(5, 5, std::chrono::seconds(10));
core::RateLimitedCommandHandler guarded(admin_guard, limiter,
                                        "Slow down!", &metrics);

// 4. Pass the outermost handler to BotService
core::BotService bot(telegram, guarded, &metrics);
```

Processing order: **rate limit → admin check → command routing**.

---

## 📊 Observability

Expose a health endpoint for monitoring and container orchestration:

```cpp
#include "vertel/runtime/health_server.hpp"
#include "vertel/runtime/metrics.hpp"

runtime::MetricsRegistry metrics;
runtime::HealthServer health(metrics, 8080);
health.Start();

// ... run bot ...

health.Stop();
```

### Endpoints

| Endpoint | Response | Purpose |
|:---------|:---------|:--------|
| `GET /healthz` | `200 ok` | Liveness probe |
| `GET /metrics` | Prometheus-style counters | Observability |

### Exposed Metrics

| Metric | Description |
|:-------|:------------|
| `vertel_updates_processed_total` | Total updates polled |
| `vertel_messages_sent_total` | Total messages sent |
| `vertel_handler_failures_total` | Handler processing errors |
| `vertel_rate_limit_rejections_total` | Rate-limited requests |

---

## 📝 Structured Logging

```cpp
#include "vertel/runtime/logger.hpp"

runtime::Logger logger;
logger.Log(runtime::LogLevel::kInfo, "bot_starting",
           {{"component", "app"}, {"version", "0.1.0"}});
```

Output (JSON to `stdout`):

```json
{"timestamp":"2025-01-15T10:30:00Z","level":"info","message":"bot_starting","component":"app","version":"0.1.0"}
```

---

## 🔁 Retry with Exponential Backoff

```cpp
#include "vertel/runtime/retry_policy.hpp"

runtime::RetryPolicy retry{.max_attempts = 5,
                            .initial_backoff = std::chrono::milliseconds(250)};

bool ok = retry.Execute(
    [&] { bot.ProcessOnce(); return true; },
    [] { return true; });  // always retryable
```

Backoff doubles each attempt: `250 ms → 500 ms → 1 s → 2 s`.

---

## 🏗️ Building from Source

### Prerequisites

```bash
# Ubuntu / Debian
sudo apt-get install build-essential cmake libcurl4-openssl-dev

# macOS
brew install cmake curl

# Fedora
sudo dnf install gcc-c++ cmake libcurl-devel
```

### Build & Test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

### CMake Options

| Option | Default | Description |
|:-------|:--------|:------------|
| `VERTEL_BUILD_TESTS` | `ON` | Build the test suite |
| `VERTEL_BUILD_EXAMPLES` | `ON` | Build `examples/basic_bot` |

> **Note:** libcurl is detected automatically. Without it, the library builds in sample-only mode (no live Telegram HTTP calls).

---

## 📦 Using VerTel as a Library

<details>
<summary><strong>Option 1 — CMake Install</strong></summary>

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build --prefix /usr/local
```

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_bot LANGUAGES CXX)

find_package(vertel CONFIG REQUIRED)

add_executable(my_bot main.cpp)
target_link_libraries(my_bot PRIVATE vertel::vertel)
```

</details>

<details>
<summary><strong>Option 2 — FetchContent</strong></summary>

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

</details>

<details>
<summary><strong>Option 3 — Vendored Subdirectory</strong></summary>

Copy the `vertel-bot/` directory into your project:

```cmake
add_subdirectory(vendor/vertel-bot)
target_link_libraries(my_bot PRIVATE vertel)
```

</details>

---

## 🪟 Using VerTel with Visual Studio

### Option A — Open with CMake (recommended)

Visual Studio 2022 natively supports CMake projects:

1. **File → Open → Folder** → select the `vertel-bot` root directory
2. VS will auto-detect `CMakeLists.txt` and configure the project
3. Select **Release** / **x64** from the configuration dropdown
4. **Build → Build All** (`Ctrl+Shift+B`)

> **Tip:** If you need libcurl, install it via vcpkg first:
> ```powershell
> vcpkg install curl:x64-windows-static
> ```
> Then set the CMake toolchain in VS: **Project → CMake Settings** → add `-DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake`.

### Option B — Pre-built SDK from Releases

1. Download `vertel-v0.9.0-windows-x64-msvc.zip` from [Releases](https://github.com/zhugez/vertelt-bot/releases)
2. Extract to a folder, e.g. `C:\libs\vertel-sdk`
3. In your Visual Studio project:
   - **View → Property Manager** → right-click your config → **Add Existing Property Sheet**
   - Select `vertel.props` from the extracted SDK folder
4. That's it — headers and libs are configured automatically

Alternatively, configure manually via **Project → Properties**:

| Setting | Value |
|:--------|:------|
| **C/C++ → Additional Include Directories** | `C:\libs\vertel-sdk\include` |
| **C/C++ → Language Standard** | `ISO C++20 (/std:c++20)` |
| **C/C++ → Preprocessor Definitions** | Add `VERTEL_HAS_LIBCURL=1` |
| **Linker → Additional Library Directories** | `C:\libs\vertel-sdk\lib\x64` |
| **Linker → Additional Dependencies** | `vertel_core.lib;vertel_runtime.lib;vertel_adapters.lib;vertel_platform.lib;libcurl.lib;zlib.lib;ws2_32.lib;Crypt32.lib;Wldap32.lib;Normaliz.lib;Iphlpapi.lib;Secur32.lib` |

### Option C — vcpkg + Visual Studio

```powershell
# Install VerTel and its dependencies
vcpkg install curl:x64-windows-static

# Clone and build VerTel
git clone https://github.com/zhugez/vertelt-bot.git
cd vertelt-bot
cmake -S . -B build -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_INSTALLATION_ROOT%/scripts/buildsystems/vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static
cmake --build build --config Release
cmake --install build --prefix C:\libs\vertel-sdk
```

Then add the property sheet or configure manually as shown in Option B.

### Option D — Single-Header Library (`vertel.hpp`)

For the absolute easiest integration, you can use the amalgamated single-header version. This requires no build system and no external dependencies (it automatically uses WinHTTP on Windows).

1. Download `vertel.hpp` from the [Releases](https://github.com/zhugez/vertelt-bot/releases) page.
2. Drop `vertel.hpp` directly into your project directory.
3. In exactly **ONE** source file (e.g., `main.cpp`), define the implementation macro before including it:

```cpp
#define VERTEL_IMPLEMENTATION
#include "vertel.hpp"

int main() { ... }
```

4. Everywhere else, simply `#include "vertel.hpp"` without the macro.
5. **Compilation (Windows/MSVC example):**
   ```cmd
   cl /EHsc /std:c++20 main.cpp winhttp.lib psapi.lib ws2_32.lib advapi32.lib
   ```
   *Note: Linking `winhttp.lib`, `psapi.lib`, `ws2_32.lib`, and `advapi32.lib` is required on Windows.*

---

## ▶️ Running the Reference Bot

### Telegram Mode

```bash
export TELEGRAM_BOT_TOKEN="<token-from-botfather>"
./build/vertel_basic_bot
```

### Sample Mode (no Telegram needed)

```bash
VERTEL_INJECT_SAMPLE_START=1 ./build/vertel_basic_bot
```

Injects a fake `/start` update so you can verify the bot processes messages without a real Telegram connection.

### Docker

```bash
# Docker Compose
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

echo "TELEGRAM_BOT_TOKEN=your-token" | sudo tee /etc/vertel-bot/vertel-bot.env
sudo systemctl enable --now vertel-bot
```

---

## ⚙️ Configuration

All configuration is via environment variables. When using VerTel as a library you pass values directly in code; the reference bot reads these:

| Variable | Default | Description |
|:---------|:--------|:------------|
| `TELEGRAM_BOT_TOKEN` | *(required)* | Bot token from [@BotFather](https://t.me/BotFather) |
| `VERTEL_INJECT_SAMPLE_START` | `0` | Set `1` to inject a fake `/start` update |
| `VERTEL_TELEGRAM_LONG_POLL_TIMEOUT_SECONDS` | `25` | Telegram long-poll timeout |
| `VERTEL_TELEGRAM_REQUEST_TIMEOUT_SECONDS` | `35` | HTTP request timeout |
| `VERTEL_POLL_MAX_ATTEMPTS` | `5` | Max retry attempts per poll cycle |
| `VERTEL_POLL_INITIAL_BACKOFF_MS` | `250` | Initial retry backoff (doubles each attempt) |
| `VERTEL_LOOP_SLEEP_MS` | `50` | Sleep between poll cycles |
| `VERTEL_RATE_LIMIT_CAPACITY` | `5` | Token bucket capacity per chat |
| `VERTEL_RATE_LIMIT_REFILL_TOKENS` | `5` | Tokens refilled per period |
| `VERTEL_RATE_LIMIT_REFILL_SECONDS` | `10` | Refill period in seconds |
| `ADMIN_CHAT_IDS` | *(empty)* | Comma-separated allowed chat IDs (empty = all) |
| `VERTEL_HTTP_PORT` | `8080` | Health/metrics server port (`≤0` to disable) |

---

## 🏛️ Architecture

```
your_main.cpp
    │
    ▼
┌───────────────────┐
│       core        │  BotService, CommandHandler, CommandRouter,
│                   │  RateLimiter, AdminWhitelist
└────────┬──────────┘
         │
    ┌────┴────┐
    │         │
┌───▼───┐ ┌──▼────────┐
│adapters│ │  runtime   │  Logger, RetryPolicy, Metrics,
│Telegram│ │  Health,   │  ShutdownSignal, HealthServer
│Client  │ │  Shutdown  │
└───┬───┘ └────────────┘
    │
┌───▼──────┐
│ platform │  Config::FromEnv()
└──────────┘
```

| Layer | Namespace | Responsibility |
|:------|:----------|:---------------|
| **core** | `vertel::core` | Bot logic, command handling, middleware, message types |
| **adapters** | `vertel::adapters::telegram` | Telegram HTTP client (long polling + `sendMessage`) |
| **runtime** | `vertel::runtime` | Logging, metrics, health server, retry, graceful shutdown |
| **platform** | `vertel::platform` | Environment-based configuration parsing |

---

## 🔬 API Reference

### Core Types

```cpp
namespace vertel::core {
  struct Update { int64_t update_id; int64_t chat_id; std::string text; };
  struct OutgoingMessage { int64_t chat_id; std::string text; };
}
```

### Key Classes

| Class | Header | Purpose |
|:------|:-------|:--------|
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
| `ShutdownSignal` | `vertel/runtime/shutdown.hpp` | SIGINT / SIGTERM handler |
| `Config` | `vertel/platform/config.hpp` | `Config::FromEnv()` reads env vars |

---

## 🧪 Testing with a Fake Gateway

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

---

## 🤝 Contributing

| Document | Description |
|:---------|:------------|
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute |
| [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) | Code of conduct |
| [SECURITY.md](SECURITY.md) | Security policy |
| [CHANGELOG.md](CHANGELOG.md) | Version history |
| [OPERATIONS.md](OPERATIONS.md) | Runtime troubleshooting |

---

## 📄 License

MIT License — see [LICENSE](LICENSE) for details.
