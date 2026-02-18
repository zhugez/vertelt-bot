# VerTel Bot

Runnable production-ready C++ Telegram bot template with layered architecture intact.

## What v1 Includes

- Command router for `/start`, `/help`, `/ping`
- Admin whitelist middleware via `ADMIN_CHAT_IDS`
- Middleware-style per-chat token bucket rate limiter (in-memory)
- Telegram Bot API adapter over libcurl (`getUpdates` + `sendMessage`) with robust `nlohmann/json` parsing
- Polling loop with update offset tracking and retry/backoff
- Built-in HTTP observability server (`/healthz`, `/metrics`)
- Docker + docker-compose template
- systemd unit in `deploy/vertel-bot.service`

## Architecture

- `app/`: Composition root and process entrypoint
- `core/`: Command handlers, router, middleware, bot orchestration
- `adapters/`: Telegram transport adapter (libcurl)
- `runtime/`: Logging, retry, graceful shutdown
- `third_party/`: Vendored third-party headers (`nlohmann/json.hpp`)
- `platform/`: Environment config
- `tests/`: Fast verification
- `deploy/`: Deployment artifacts

## Build (Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libcurl4-openssl-dev

cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Run

```bash
export TELEGRAM_BOT_TOKEN="<bot-token>"
export VERTEL_INJECT_SAMPLE_START=0
./build/vertel_bot
```

Local sample mode (no Telegram network calls):

```bash
VERTEL_INJECT_SAMPLE_START=1 ./build/vertel_bot
```

## Environment Variables

Required for real Telegram mode:

- `TELEGRAM_BOT_TOKEN`: Bot token from BotFather

Optional:

- `VERTEL_INJECT_SAMPLE_START` (default `0`): `1` emits one synthetic `/start` update and exits
- `VERTEL_TELEGRAM_LONG_POLL_TIMEOUT_SECONDS` (default `25`)
- `VERTEL_TELEGRAM_REQUEST_TIMEOUT_SECONDS` (default `35`)
- `VERTEL_POLL_MAX_ATTEMPTS` (default `5`)
- `VERTEL_POLL_INITIAL_BACKOFF_MS` (default `250`)
- `VERTEL_LOOP_SLEEP_MS` (default `50`)
- `VERTEL_RATE_LIMIT_CAPACITY` (default `5`)
- `VERTEL_RATE_LIMIT_REFILL_TOKENS` (default `5`)
- `VERTEL_RATE_LIMIT_REFILL_SECONDS` (default `10`)
- `ADMIN_CHAT_IDS` (default empty): comma-separated Telegram chat IDs allowed to run commands, e.g. `1234,-100555666`
- `VERTEL_HTTP_PORT` (default `8080`, set `<=0` to disable built-in HTTP server)

## Health and Metrics

- `GET /healthz` returns `200 OK` with body `ok`
- `GET /metrics` returns plaintext counters:
  - `vertel_updates_processed_total`
  - `vertel_messages_sent_total`
  - `vertel_handler_failures_total`
  - `vertel_rate_limit_rejections_total`

## Docker

```bash
docker compose up --build
```

Set `TELEGRAM_BOT_TOKEN` in shell or `.env` before running compose.

## systemd

Unit file: `deploy/vertel-bot.service`

Example install:

```bash
sudo install -D -m 0755 build/vertel_bot /opt/vertel-bot/vertel_bot
sudo install -D -m 0644 deploy/vertel-bot.service /etc/systemd/system/vertel-bot.service
sudo install -D -m 0640 /dev/stdin /etc/vertel-bot/vertel-bot.env <<'ENV'
TELEGRAM_BOT_TOKEN=<bot-token>
ENV
sudo systemctl daemon-reload
sudo systemctl enable --now vertel-bot
```

See `OPERATIONS.md` for troubleshooting.
