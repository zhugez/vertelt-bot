# Consuming VerTel as a Library

VerTel supports three consumption methods: CMake `find_package`, CMake `FetchContent`/`add_subdirectory`, and pkg-config.

---

## 1. CMake `find_package` (installed)

Install VerTel first:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build --prefix /usr/local
```

Consumer `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_bot LANGUAGES CXX)

find_package(vertel CONFIG REQUIRED)

add_executable(my_bot main.cpp)
target_link_libraries(my_bot PRIVATE vertel::vertel)
```

You can also link individual components:

```cmake
target_link_libraries(my_bot PRIVATE vertel::core vertel::runtime)
```

---

## 2. CMake `FetchContent` (no pre-install)

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_bot LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  vertel
  GIT_REPOSITORY https://github.com/AiverTel/vertel-bot.git
  GIT_TAG        master   # or a release tag like v0.1.0
)
FetchContent_MakeAvailable(vertel)

add_executable(my_bot main.cpp)
target_link_libraries(my_bot PRIVATE vertel::vertel)
```

When consumed via `FetchContent`, tests, examples, and install rules are
automatically disabled (they only build when VerTel is the top-level project).

---

## 3. CMake `add_subdirectory` (vendored)

Clone or add VerTel as a git submodule:

```bash
git submodule add https://github.com/AiverTel/vertel-bot.git third_party/vertel
```

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_bot LANGUAGES CXX)

add_subdirectory(third_party/vertel)

add_executable(my_bot main.cpp)
target_link_libraries(my_bot PRIVATE vertel::vertel)
```

---

## 4. pkg-config (non-CMake build systems)

After installing VerTel, a `vertel.pc` file is placed in `<prefix>/lib/pkgconfig/`.

### Meson (`meson.build`)

```meson
vertel_dep = dependency('vertel')
executable('my_bot', 'main.cpp', dependencies: vertel_dep)
```

### Plain Makefile

```makefile
CXXFLAGS += $(shell pkg-config --cflags vertel)
LDFLAGS  += $(shell pkg-config --libs vertel)

my_bot: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
```

### Shell / one-liner

```bash
g++ -std=c++20 main.cpp $(pkg-config --cflags --libs vertel) -o my_bot
```

---

## Available targets

| Target               | Description                          |
|----------------------|--------------------------------------|
| `vertel::vertel`     | Umbrella — links all components      |
| `vertel::core`       | Bot service / command routing         |
| `vertel::runtime`    | Health server, logging, shutdown      |
| `vertel::adapters`   | Telegram HTTP client                  |
| `vertel::platform`   | Config loading, environment helpers   |
