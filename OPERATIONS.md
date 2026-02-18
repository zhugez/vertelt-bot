# OPERATIONS

## Startup Checklist

1. Build with CMake and run tests.
2. Set `TELEGRAM_BOT_TOKEN`.
3. Ensure `VERTEL_INJECT_SAMPLE_START=0` for production.
4. Start the process (`./build/vertel_bot`, Docker, or systemd).

## Polling and Reliability

- Uses Telegram long polling (`getUpdates`) with offset tracking.
- On transient failures, the app retries each loop using `RetryPolicy` exponential backoff.
- Logs are structured JSON to stdout.

## Command Surface

- `/start`: welcome
- `/help`: lists available commands
- `/ping`: health-style reply (`pong`)

## Rate Limiting

In-memory token bucket per `chat_id`:

- `VERTEL_RATE_LIMIT_CAPACITY`
- `VERTEL_RATE_LIMIT_REFILL_TOKENS`
- `VERTEL_RATE_LIMIT_REFILL_SECONDS`

Exceeded requests receive `Rate limit exceeded. Please slow down.`

## Admin Authorization

- `ADMIN_CHAT_IDS` is a comma-separated whitelist of Telegram chat IDs.
- If `ADMIN_CHAT_IDS` is unset or empty, all chats are allowed.
- Non-whitelisted chats receive `Unauthorized.`.

## Health and Metrics Endpoint

- The process exposes an HTTP endpoint on `VERTEL_HTTP_PORT` (default `8080`, set `<=0` to disable).
- `GET /healthz` returns `ok`.
- `GET /metrics` returns:
  - `vertel_updates_processed_total`
  - `vertel_messages_sent_total`
  - `vertel_handler_failures_total`
  - `vertel_rate_limit_rejections_total`

## Key Env Vars

- `TELEGRAM_BOT_TOKEN` (required for real Telegram mode)
- `VERTEL_INJECT_SAMPLE_START` (`0` production, `1` synthetic test update)
- `VERTEL_TELEGRAM_LONG_POLL_TIMEOUT_SECONDS`
- `VERTEL_TELEGRAM_REQUEST_TIMEOUT_SECONDS`
- `VERTEL_POLL_MAX_ATTEMPTS`
- `VERTEL_POLL_INITIAL_BACKOFF_MS`
- `VERTEL_LOOP_SLEEP_MS`
- `VERTEL_RATE_LIMIT_CAPACITY`
- `VERTEL_RATE_LIMIT_REFILL_TOKENS`
- `VERTEL_RATE_LIMIT_REFILL_SECONDS`
- `ADMIN_CHAT_IDS`
- `VERTEL_HTTP_PORT`

## Troubleshooting

- Symptom: no responses in Telegram
  - Verify bot token validity and process logs for `poll_iteration_failed`.
  - Confirm only one polling consumer is running for the bot token.
  - Send `/ping` to validate command path.

- Symptom: frequent retry warnings
  - Check network egress and DNS from host/container.
  - Increase `VERTEL_TELEGRAM_REQUEST_TIMEOUT_SECONDS` if needed.

- Symptom: users see rate limit message too often
  - Increase `VERTEL_RATE_LIMIT_CAPACITY` and/or refill settings.

- Symptom: non-admin users cannot run commands
  - Verify `ADMIN_CHAT_IDS` contains the intended chat IDs.

- Symptom: exits immediately
  - Ensure `VERTEL_INJECT_SAMPLE_START=0` in production.

- Symptom: `/healthz` or `/metrics` unavailable
  - Ensure `VERTEL_HTTP_PORT` is set to a free port greater than `0`.
