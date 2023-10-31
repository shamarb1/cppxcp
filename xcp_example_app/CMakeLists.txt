project(xcp_example_app)

# Sources

add_executable(${PROJECT_NAME} src/business_logic.cpp src/example_app.cpp src/xcp_slave_core_int.cpp)

# Compiler

# `-g` flag is needed for generating of A2L and S19 files.
target_compile_options(${PROJECT_NAME} PRIVATE -g)

# Modules

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../xcp_slave/cmake)
include(xcp_integration)

# Libraries

set(XCP_SLAVE_BASE_DIR ${PROJECT_SOURCE_DIR}/../xcp_slave)
set(XCP_SLAVE_INCLUDE_DIR ${XCP_SLAVE_BASE_DIR}/include)
set(XCP_SLAVE_LIBXCP_SLAVE ${PROJECT_SOURCE_DIR}/../build_local/xcp_slave/libxcp_slave.so)

target_link_library_xcp(
  ${PROJECT_NAME}
  UDP_PORT
  30000
  CALIBRATION
  A2L_GENERATION
)

# Turn on QNX specific code

if(IS_QNX)
  target_compile_definitions(${PROJECT_NAME} PRIVATE QNX_SPECIFIC_CODE)
endif()

# Run clang-format

target_clang_format(${PROJECT_NAME})

# Run clang-tidy

target_clang_tidy(${PROJECT_NAME})
