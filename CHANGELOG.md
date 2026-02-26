# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog and this project follows Semantic Versioning.

## [0.1.0] - 2026-02-26

### Added

- Core Telegram bot framework with C++20 (bot service, command routing)
- Runtime components: health server, structured logging, retry policy, graceful shutdown
- Telegram HTTP adapter with libcurl (optional — builds without it)
- Platform config loading and environment helpers
- Professional CMake build system with `find_package`, `FetchContent`, and `add_subdirectory` support
- ALIAS targets (`vertel::core`, `vertel::runtime`, `vertel::adapters`, `vertel::platform`, `vertel::vertel`)
- Generated version header (`vertel_version.h`)
- pkg-config support (`vertel.pc`)
- GNUmakefile wrapper for Crypto++-style UX (`make && sudo make install`)
- Build-tree export for `find_package` without installing
- Sanitizer builds (ASan, UBSan) and coverage via GNUmakefile
- CI with compiler matrix (GCC + Clang), formatting, and sanitizers
- Tagged release workflow with binary artifacts
- Open-source community health files and governance docs
- Consumer documentation with 4 integration methods

## [Unreleased]
