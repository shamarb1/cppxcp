project(xcp_slave)

# Sources

add_library(
  ${PROJECT_NAME} SHARED
  lib/xcp.c
  lib/xcp_globals.c
  lib/xcp_slave_cal_page_file_io.c
  lib/xcp_slave_core.c
  lib/xcp_slave_core_calibration.c
  lib/xcp_slave_core_common.c
  lib/xcp_slave_log.c
)

# API interface

target_include_directories(${PROJECT_NAME} PUBLIC include)

# Run clang-format

target_clang_format(${PROJECT_NAME})

# Run clang-tidy

target_clang_tidy(${PROJECT_NAME})
