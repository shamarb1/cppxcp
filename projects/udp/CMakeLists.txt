project(udp)

# Sources

add_library(${PROJECT_NAME} lib/udp_socket.cpp lib/udp_socket_types.cpp)

# Includes

target_include_directories(${PROJECT_NAME} PUBLIC include PRIVATE ${CMAKE_SOURCE_DIR}/projects/common/include)

# Run clang-format

target_clang_format(${PROJECT_NAME})

# Run clang-tidyudp

target_clang_tidy(${PROJECT_NAME})

# Run unit tests

target_unittests(
  ${PROJECT_NAME}
  SOURCES
  ${CMAKE_SOURCE_DIR}/projects/common/src/gtest_main.cpp
  tests/test_udp_server.cpp
  tests/test_udp_socket.cpp
  INCLUDES
  lib
)
