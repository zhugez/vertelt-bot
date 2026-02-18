cmake_minimum_required(VERSION 3.20)
project(VerTelBot VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VERTEL_BUILD_TESTS "Build tests" ON)
find_package(CURL QUIET)
find_package(Threads REQUIRED)

add_library(vertel_core
  core/src/bot_service.cpp
)
target_include_directories(vertel_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/core/include
  ${CMAKE_CURRENT_SOURCE_DIR}/adapters/include
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime/include
)

add_library(vertel_runtime
  runtime/src/health_server.cpp
  runtime/src/logger.cpp
  runtime/src/retry_policy.cpp
  runtime/src/shutdown.cpp
)
target_link_libraries(vertel_runtime PUBLIC Threads::Threads)
target_include_directories(vertel_runtime PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/core/include
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime/include
)

add_library(vertel_adapters
  adapters/src/telegram_client.cpp
)
target_include_directories(vertel_adapters PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/adapters/include
  ${CMAKE_CURRENT_SOURCE_DIR}/core/include
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)
if(CURL_FOUND)
  target_link_libraries(vertel_adapters PUBLIC CURL::libcurl)
  target_compile_definitions(vertel_adapters PUBLIC VERTEL_HAS_LIBCURL=1)
else()
  message(WARNING "libcurl development files not found. Building without live Telegram HTTP support.")
  target_compile_definitions(vertel_adapters PUBLIC VERTEL_HAS_LIBCURL=0)
endif()

add_library(vertel_platform
  platform/src/config.cpp
)
target_include_directories(vertel_platform PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/include
)

add_executable(vertel_bot
  app/main.cpp
)
target_link_libraries(vertel_bot PRIVATE
  vertel_core
  vertel_runtime
  vertel_adapters
  vertel_platform
)

include(CTest)
if(BUILD_TESTING AND VERTEL_BUILD_TESTS)
  add_executable(core_tests
    tests/test_bot_service.cpp
  )
  target_link_libraries(core_tests PRIVATE
    vertel_core
    vertel_runtime
    vertel_adapters
  )
  add_test(NAME core_tests COMMAND core_tests)
endif()
