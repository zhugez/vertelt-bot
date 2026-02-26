# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog and this project follows Semantic Versioning.

## [Unreleased]

## [0.9.0] - 2026-02-27

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
- CI with compiler matrix (GCC + Clang + MSVC), formatting, and sanitizers
- Windows (MSVC) support with Winsock2 ported health server
- Visual Studio-friendly release with `.props` property sheet (x64 + x86)
- Consumer documentation with 5 integration methods (VS property sheet, CMake find_package, FetchContent, add_subdirectory, pkg-config)
- Tagged release workflow with binary artifacts (Linux + Windows)
- Open-source community health files and governance docs
