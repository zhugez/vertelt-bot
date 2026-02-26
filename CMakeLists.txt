cmake_minimum_required(VERSION 3.20)
project(vertel VERSION 0.1.0 LANGUAGES CXX)

# Detect whether vertel is the top-level project or embedded via add_subdirectory
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(VERTEL_IS_TOP_LEVEL ON)
else()
  set(VERTEL_IS_TOP_LEVEL OFF)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VERTEL_INSTALL       "Generate install targets"  ${VERTEL_IS_TOP_LEVEL})
option(VERTEL_BUILD_TESTS   "Build tests"               ${VERTEL_IS_TOP_LEVEL})
option(VERTEL_BUILD_EXAMPLES "Build runnable examples"  ${VERTEL_IS_TOP_LEVEL})

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(CURL QUIET)
find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
#  Generate version header
# ---------------------------------------------------------------------------
configure_file(
  cmake/vertel_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/vertel/vertel_version.h
  @ONLY
)

# ---------------------------------------------------------------------------
#  Core library
# ---------------------------------------------------------------------------
add_library(vertel_core
  core/src/bot_service.cpp
)
add_library(vertel::core ALIAS vertel_core)
target_compile_features(vertel_core PUBLIC cxx_std_20)
target_include_directories(vertel_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ---------------------------------------------------------------------------
#  Runtime library
# ---------------------------------------------------------------------------
add_library(vertel_runtime
  runtime/src/health_server.cpp
  runtime/src/logger.cpp
  runtime/src/retry_policy.cpp
  runtime/src/shutdown.cpp
)
add_library(vertel::runtime ALIAS vertel_runtime)
target_compile_features(vertel_runtime PUBLIC cxx_std_20)
target_link_libraries(vertel_runtime PUBLIC Threads::Threads)
target_include_directories(vertel_runtime PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ---------------------------------------------------------------------------
#  Adapters library
# ---------------------------------------------------------------------------
add_library(vertel_adapters
  adapters/src/telegram_client.cpp
)
add_library(vertel::adapters ALIAS vertel_adapters)
target_compile_features(vertel_adapters PUBLIC cxx_std_20)
target_include_directories(vertel_adapters PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(vertel_adapters PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party>
)
if(CURL_FOUND)
  target_link_libraries(vertel_adapters PUBLIC CURL::libcurl)
  target_compile_definitions(vertel_adapters PUBLIC VERTEL_HAS_LIBCURL=1)
else()
  message(WARNING "libcurl development files not found. Building without live Telegram HTTP support.")
  target_compile_definitions(vertel_adapters PUBLIC VERTEL_HAS_LIBCURL=0)
endif()

# ---------------------------------------------------------------------------
#  Platform library
# ---------------------------------------------------------------------------
add_library(vertel_platform
  platform/src/config.cpp
)
add_library(vertel::platform ALIAS vertel_platform)
target_compile_features(vertel_platform PUBLIC cxx_std_20)
target_include_directories(vertel_platform PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ---------------------------------------------------------------------------
#  Umbrella INTERFACE library
# ---------------------------------------------------------------------------
add_library(vertel INTERFACE)
add_library(vertel::vertel ALIAS vertel)
target_link_libraries(vertel INTERFACE
  vertel_core
  vertel_runtime
  vertel_adapters
  vertel_platform
)
target_compile_features(vertel INTERFACE cxx_std_20)

# ---------------------------------------------------------------------------
#  Examples
# ---------------------------------------------------------------------------
if(VERTEL_BUILD_EXAMPLES)
  add_executable(vertel_basic_bot
    examples/basic_bot/main.cpp
  )
  target_link_libraries(vertel_basic_bot PRIVATE vertel::vertel)
endif()

# ---------------------------------------------------------------------------
#  Install
# ---------------------------------------------------------------------------
if(VERTEL_INSTALL)
  set(VERTEL_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/vertel")

  # Compiled targets
  install(TARGETS
    vertel_core
    vertel_runtime
    vertel_adapters
    vertel_platform
    EXPORT vertelTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}  COMPONENT vertel-dev
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}  COMPONENT vertel-dev
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  COMPONENT vertel-dev
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  # INTERFACE umbrella target (separate — no ARCHIVE/LIBRARY/RUNTIME destinations)
  install(TARGETS vertel
    EXPORT vertelTargets
    COMPONENT vertel-dev
  )

  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT vertel-dev
  )

  # Generated version header
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/include/vertel/vertel_version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vertel
    COMPONENT vertel-dev
  )

  install(EXPORT vertelTargets
    FILE vertelTargets.cmake
    NAMESPACE vertel::
    DESTINATION ${VERTEL_INSTALL_CMAKEDIR}
    COMPONENT vertel-dev
  )

  # Build-tree export (allows find_package from build dir without installing)
  export(EXPORT vertelTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/vertelTargets.cmake
    NAMESPACE vertel::
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
    COMPONENT vertel-dev
  )

  # pkg-config
  if(CURL_FOUND)
    set(VERTEL_PC_REQUIRES "libcurl")
  else()
    set(VERTEL_PC_REQUIRES "")
  endif()
  configure_file(
    cmake/vertel.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/vertel.pc
    @ONLY
  )
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/vertel.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    COMPONENT vertel-dev
  )
endif()

# ---------------------------------------------------------------------------
#  Tests
# ---------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING AND VERTEL_BUILD_TESTS)
  add_executable(core_tests
    tests/test_bot_service.cpp
  )
  target_compile_features(core_tests PRIVATE cxx_std_20)
  target_link_libraries(core_tests PRIVATE vertel::vertel)
  add_test(NAME core_tests COMMAND core_tests)
endif()
