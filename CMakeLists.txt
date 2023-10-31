cmake_minimum_required(VERSION 3.14.0)
project(xcp)

# Preconditions

option(IS_QNX "The flag is set to TRUE when current OS is QNX")
if(${CMAKE_SYSTEM_NAME} STREQUAL "QNX")
  set(IS_QNX TRUE)
endif()

option(IS_LINUX "The flag is set to TRUE when current OS is Linux")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(IS_LINUX TRUE)
endif()

if(NOT (IS_LINUX OR IS_QNX))
  message(FATAL_ERROR "XCP project is not supposed to be build on other OS than Linux and QNX.")
endif()

# Options

set(TOOLS "" CACHE STRING "The list of development tools.")

set(PROJECTS "" CACHE STRING "The list of the enabled projects scheduled to build.")

set(TESTS "" CACHE STRING "The list of the tests scheduled to run after build.")

option(LOCAL_BUILD "The flag specifies that the build is compilated for Ubuntu PC." OFF)
if(LOCAL_BUILD)
  add_compile_definitions(LOCAL_BUILD)
else()
  set(TOOLS "")
  set(PROJECTS "xcp")
  set(TESTS "unit")
endif()

# Option aliases

function(
  replace_alias
  ARG_LIST
  ARG_ALIAS
  ARG_STRING
)
  list(TRANSFORM ${ARG_LIST} REPLACE "^${ARG_ALIAS}$" "${ARG_STRING}")
  set(${ARG_LIST} ${${ARG_LIST}} PARENT_SCOPE)
endfunction()

replace_alias(TOOLS "all" "clang-format;cmake-format;clang-tidy;bug-finder;code-prover;lcov")

replace_alias(PROJECTS "xcp_example_app" "xcp_slave;xcp_example_app")
replace_alias(PROJECTS "xcp" "xcp;udp")
replace_alias(PROJECTS "all" "xcp_slave;xcp_example_app;udp_socket")

replace_alias(TESTS "all" "unit")

# Language

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler

# GCC args. Turn on lots of warnings and all warnings as errors.
add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
  -Werror
)

if(LOCAL_BUILD)
  # GCC args. Turn off optimization and add debug flags.
  add_compile_options(-O0 -ggdb)
  set(CMAKE_BUILD_TYPE Debug) # Implicitly adds '-g' flag.
else()
  set(CMAKE_BUILD_TYPE Release) # Implicitly adds '-O3 -DNDEBUG' flags.
endif()

# Modules

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
# Utilities
include(colorized_messages)
# Mandatory
include(enabled_projects)
# Tools
include(clang_format)
include(cmake_format)
include(clang_tidy)
include(polyspace_tools)
# Tests
include(unittests)

# Projects

add_project(xcp_slave)
add_project(xcp_example_app)
add_project(udp projects/udp)
add_project(xcp projects/xcp)

# Check the final list of enabled projects
get_property(ENABLED_PROJECTS_VAR GLOBAL PROPERTY ENABLED_PROJECTS)
if(NOT ENABLED_PROJECTS_VAR)
  message(YELLOW_BOLD
          "No project has been set. Set any project using CMake variable 'PROJECTS' . E.g. '-DPROJECTS=xcp_slave'."
  )
endif()

# Run cmake-format

cmake_format()

# Run Polyspace analysis

# Run only if at least one project is enabled.
if(ENABLED_PROJECTS_VAR)
  polyspace_analysis(
    BUG_FINDER
    CODE_PROVER
    POLYSPACE_INSTALLATION_DIR
    "/usr/local/Polyspace/R2022b"
  )
endif()
