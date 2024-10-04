cmake_minimum_required(VERSION 3.20)

project(FzNet VERSION 0.0.0 LANGUAGES CXX)

set(FzNet_MAIN_PROJECT OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(FzNet_MAIN_PROJECT ON)
endif()

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(FZ_NET_PUBLIC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

set(FZ_NET_ASIO_INCLUDE_DIR "/opt/homebrew/Cellar/asio/1.30.2/include")
if (NOT FZ_NET_ASIO_INCLUDE_DIR)
    message(FATAL_ERROR "Please set FZ_NET_ASIO_INCLUDE_DIR to the asio include directory")
endif()

set(FZ_NET_EXTERNAL_INCLUDE_DIR ${FZ_NET_EXTERNAL_INCLUDE_DIR} ${FZ_NET_ASIO_INCLUDE_DIR})

add_subdirectory(src)
add_subdirectory(test)

set(FZ_NET_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

install(
  EXPORT fz_net-targets
  FILE fz_net-targets.cmake
  NAMESPACE fz::
  DESTINATION ${FZ_NET_INSTALL_PREFIX}/lib/cmake/fz_net
)

# install include
install(
  DIRECTORY ${FZ_NET_PUBLIC_INCLUDE_DIR}
  DESTINATION ${FZ_NET_INSTALL_PREFIX}/include
)

# install config files
include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/fz_net-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/fz_net-config.cmake
  INSTALL_DESTINATION ${FZ_NET_INSTALL_PREFIX}/lib/cmake/fz_net
)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/fz_net-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)
install(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/fz_net-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/fz_net-config-version.cmake
  DESTINATION ${FZ_NET_INSTALL_PREFIX}/lib/cmake/fz_net
)

# export targets
export(
  EXPORT fz_net-targets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/fz_net-targets.cmake
  NAMESPACE fz::
)
