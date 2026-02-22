# Consuming VerTel As A CMake Package

Install VerTel first:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cmake --install build --prefix /usr/local
```

Minimal consumer `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(vertel_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vertel CONFIG REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE vertel::vertel)
```
