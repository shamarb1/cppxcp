project(xcp)

# Sources

set(PROTOCOL_DIR lib/protocol_layer)
set(TRANSPORT_DIR lib/transport_layer/ethernet)

add_library(${PROJECT_NAME} ${TRANSPORT_DIR}/xcp_transport_server.cpp ${TRANSPORT_DIR}/xcp_transport_types.cpp)

# Includes

target_include_directories(
  ${PROJECT_NAME} PUBLIC include PRIVATE ${CMAKE_SOURCE_DIR}/projects/common/include ${PROTOCOL_DIR} ${TRANSPORT_DIR}
)

# Libraries

target_link_libraries(${PROJECT_NAME} udp)

# Run clang-format

target_clang_format(${PROJECT_NAME})

# Run clang-tidyudp

target_clang_tidy(${PROJECT_NAME})

# Run unit tests

target_unittests(
  ${PROJECT_NAME}
  SOURCES
  ${CMAKE_SOURCE_DIR}/projects/common/src/gtest_main.cpp
  tests/test_xcp_transport_layer.cpp
  INCLUDES
  transport_layer/ethernet
)
