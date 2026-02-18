# Roadmap (Telegram-core first)

## Near term (v0.2.x)

- Harden Telegram Bot API correctness and edge-case handling
- Replace/upgrade parser paths with stricter typed decoding
- Improve test matrix for update/message variants and failure modes
- Tighten env/config validation + safer defaults

## Mid term (v0.3.x)

- Webhook mode parity with polling mode
- Better middleware contracts (auth, rate-limit, retries, idempotency)
- Transport abstraction cleanups to support pluggable HTTP backends
- Benchmarks + performance profiling for high-throughput bot workloads

## Long term (v1.0)

- Stabilize a reusable **library-grade API surface**
- Publish versioned compatibility policy for Telegram API updates
- Production profile with SLO-oriented guidance, observability, and scaling patterns
- Position VerTel as a serious evolution path for C++ Telegram bot development
