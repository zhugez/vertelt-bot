# Contributing to VerTel Bot

Thanks for contributing. We aim to keep contributions small, reviewable, and production-minded.

## Development setup

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libcurl4-openssl-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Workflow

1. Create a branch from `main`.
2. Make focused commits with clear messages.
3. Add or update tests for behavior changes.
4. Run local checks before opening a PR.

## Local checks

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

If available locally:

```bash
clang-format --dry-run --Werror $(git ls-files '*.h' '*.hpp' '*.c' '*.cc' '*.cpp')
```

## Pull request guidelines

- Explain the problem and the approach.
- Mention risks and rollback plan for operational changes.
- Keep docs updated when behavior/configuration changes.
- Ensure CI is green.

## Commit style

Use concise, imperative subject lines, for example:

- `core: harden command parsing`
- `runtime: improve retry jitter`
- `docs: document metrics contract`
