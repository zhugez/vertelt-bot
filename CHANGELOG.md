# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog and this project follows Semantic Versioning.

## [Unreleased]

### Added

- Open-source community health files and governance docs
- Expanded CI with compiler matrix, formatting, optional tidy, and sanitizers
- Tagged release workflow with binary artifacts
- Architecture and roadmap documentation
- Consumer packaging docs with `find_package(vertel CONFIG REQUIRED)` example

### Changed

- Refactored to a library-first layout with runnable example app at `examples/basic_bot/main.cpp`
- Consolidated public API headers under `include/vertel` with compatibility shim headers for prior include roots
- CMake now installs and exports package targets and generates `vertelConfig.cmake` + version metadata
