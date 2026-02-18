# Architecture

## Goal

VerTel Bot is structured for operational reliability and testability while staying lightweight.

## Layered design

```text
app -> core -> adapters/runtime -> platform
```

- `app/`: composition root and process lifecycle
- `core/`: domain-level bot flow, handlers, middleware, orchestration
- `adapters/`: external transport (Telegram API via HTTP)
- `runtime/`: cross-cutting concerns (retry, logging, shutdown, health, metrics)
- `platform/`: environment-driven configuration

## Runtime flow

1. Load config from environment.
2. Start health/metrics server.
3. Poll updates from Telegram adapter.
4. Route commands through core service and middleware.
5. Send responses through adapter.
6. Publish metrics and handle graceful shutdown.

## Design decisions

- C++20 baseline with CMake for portability and predictable builds.
- `nlohmann/json` vendored for deterministic parsing behavior.
- Polling model is preferred in template form for lower infrastructure burden.

## Failure handling

- Retry with backoff for network/transient failures.
- Graceful shutdown hooks to stop loops cleanly.
- Health and metrics endpoints for container/orchestrator integration.
