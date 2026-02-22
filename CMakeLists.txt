cmake_minimum_required(VERSION 3.20)
project(vertel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VERTEL_BUILD_TESTS "Build tests" ON)
option(VERTEL_BUILD_EXAMPLES "Build runnable examples" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(CURL QUIET)
find_package(Threads REQUIRED)

add_library(vertel_core
  core/src/bot_service.cpp
)
target_compile_features(vertel_core PUBLIC cxx_std_20)
target_include_directories(vertel_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(vertel_runtime
  runtime/src/health_server.cpp
  runtime/src/logger.cpp
  runtime/src/retry_policy.cpp
  runtime/src/shutdown.cpp
)
target_compile_features(vertel_runtime PUBLIC cxx_std_20)
target_link_libraries(vertel_runtime PUBLIC Threads::Threads)
target_include_directories(vertel_runtime PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(vertel_adapters
  adapters/src/telegram_client.cpp
)
target_compile_features(vertel_adapters PUBLIC cxx_std_20)
target_include_directories(vertel_adapters PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(vertel_adapters PRIVATE
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
target_compile_features(vertel_platform PUBLIC cxx_std_20)
target_include_directories(vertel_platform PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(vertel INTERFACE)
target_link_libraries(vertel INTERFACE
  vertel_core
  vertel_runtime
  vertel_adapters
  vertel_platform
)
target_compile_features(vertel INTERFACE cxx_std_20)

if(VERTEL_BUILD_EXAMPLES)
  add_executable(vertel_basic_bot
    examples/basic_bot/main.cpp
  )
  target_link_libraries(vertel_basic_bot PRIVATE vertel)
endif()

set(VERTEL_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/vertel")

install(TARGETS
  vertel
  vertel_core
  vertel_runtime
  vertel_adapters
  vertel_platform
  EXPORT vertelTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT vertelTargets
  FILE vertelTargets.cmake
  NAMESPACE vertel::
  DESTINATION ${VERTEL_INSTALL_CMAKEDIR}
)

set(VERTEL_WITH_CURL ${CURL_FOUND})
configure_package_config_file(
  cmake/vertelConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/vertelConfig.cmake
  INSTALL_DESTINATION ${VERTEL_INSTALL_CMAKEDIR}
)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/vertelConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/vertelConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/vertelConfigVersion.cmake
  DESTINATION ${VERTEL_INSTALL_CMAKEDIR}
)

include(CTest)
if(BUILD_TESTING AND VERTEL_BUILD_TESTS)
  add_executable(core_tests
    tests/test_bot_service.cpp
  )
  target_compile_features(core_tests PRIVATE cxx_std_20)
  target_link_libraries(core_tests PRIVATE
    vertel
  )
  add_test(NAME core_tests COMMAND core_tests)
endif()
