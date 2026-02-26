# Consuming VerTel as a Library

VerTel supports multiple consumption methods: Visual Studio (property sheet), CMake, and pkg-config.

---

## 1. Visual Studio — Console App / WinAPI (Recommended for Windows)

Download the latest `vertel-vX.Y.Z-windows-x64-msvc.zip` (or `x86`) from
[Releases](https://github.com/zhugez/vertelt-bot/releases) and extract it.

The zip contains:

```
vertel/
  include/          ← headers
  lib/x64/          ← .lib files (static libraries)
  vertel.props      ← Visual Studio property sheet
  README.md
  LICENSE
```

### Option A: Property sheet (easiest)

1. Extract the zip somewhere permanent, e.g. `C:\libs\vertel\`
2. In Visual Studio, open your Console App project
3. Go to **View > Property Manager**
4. Right-click your config (e.g. `Release | x64`) > **Add Existing Property Sheet**
5. Browse to `C:\libs\vertel\vertel.props`
6. Done — includes, libs, and C++20 are configured automatically

```cpp
// main.cpp — just include and use
#include <vertel/core/bot_service.hpp>
#include <vertel/adapters/telegram/telegram_client.hpp>
#include <vertel/runtime/shutdown.hpp>

int main() {
    vertel::adapters::telegram::TelegramClient telegram("YOUR_TOKEN", 25, 35);
    vertel::core::BotService bot(telegram);
    vertel::runtime::ShutdownSignal::Install();
    bot.Run();
    return 0;
}
```

### Option B: Manual project settings

1. **C/C++ > General > Additional Include Directories**: add `C:\libs\vertel\include`
2. **C/C++ > Language > C++ Language Standard**: set to **ISO C++20**
3. **C/C++ > Preprocessor > Preprocessor Definitions**: add `VERTEL_HAS_LIBCURL=1` (if using curl) or `VERTEL_HAS_LIBCURL=0`
4. **Linker > General > Additional Library Directories**: add `C:\libs\vertel\lib\x64` (or `x86`)
5. **Linker > Input > Additional Dependencies**: add:
   ```
   vertel_core.lib
   vertel_runtime.lib
   vertel_adapters.lib
   vertel_platform.lib
   ws2_32.lib
   ```

---

## 2. CMake `find_package` (installed)

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

## 3. CMake `FetchContent` (no pre-install)

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_bot LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  vertel
  GIT_REPOSITORY https://github.com/zhugez/vertelt-bot.git
  GIT_TAG        master   # or a release tag like v0.1.0
)
FetchContent_MakeAvailable(vertel)

add_executable(my_bot main.cpp)
target_link_libraries(my_bot PRIVATE vertel::vertel)
```

When consumed via `FetchContent`, tests, examples, and install rules are
automatically disabled (they only build when VerTel is the top-level project).

---

## 4. CMake `add_subdirectory` (vendored)

Clone or add VerTel as a git submodule:

```bash
git submodule add https://github.com/zhugez/vertelt-bot.git third_party/vertel
```

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_bot LANGUAGES CXX)

add_subdirectory(third_party/vertel)

add_executable(my_bot main.cpp)
target_link_libraries(my_bot PRIVATE vertel::vertel)
```

---

## 5. pkg-config (non-CMake build systems)

After installing VerTel, a `vertel.pc` file is placed in `<prefix>/lib/pkgconfig/`.

### Plain Makefile

```makefile
CXXFLAGS += $(shell pkg-config --cflags vertel)
LDFLAGS  += $(shell pkg-config --libs vertel)

my_bot: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
```

---

## Available targets / libraries

| Target / Library       | Description                          |
|------------------------|--------------------------------------|
| `vertel::vertel`       | Umbrella — links all components      |
| `vertel::core`         | Bot service / command routing         |
| `vertel::runtime`      | Health server, logging, shutdown      |
| `vertel::adapters`     | Telegram HTTP client                  |
| `vertel::platform`     | Config loading, environment helpers   |

On Windows (`.lib` files): `vertel_core.lib`, `vertel_runtime.lib`, `vertel_adapters.lib`, `vertel_platform.lib`.
