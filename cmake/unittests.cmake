if(unit IN_LIST TESTS)
  enable_testing()
endif()

function(target_unittests ARG_TARGET)

  if(NOT (unit IN_LIST TESTS))
    return()
  endif()

  # Parse arguments
  
  # Sometimes, tests fail during development, and the binary automatically is deleted by Make tool.
  # And it's unable to debug the binary. This flag prevents automatic tests running.
  set(OPTIONS DO_NOT_RUN)
  set(MULTI_VALUE_ARGS SOURCES INCLUDES LINKS)
  cmake_parse_arguments(
    ARG
    "${OPTIONS}"
    ""
    "${MULTI_VALUE_ARGS}"
    ${ARGN}
  )

  # Check arguments for correctness

  if(NOT ARG_TARGET)
    message(FATAL_ERROR "Target name should not be empty.")
  endif()

  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "SOURCES should have at least one source file to make an executable.")
  endif()

  # Do not run tests for release build
  if(NOT LOCAL_BUILD)
    set(ARG_DO_NOT_RUN TRUE)
  endif()

  # Set test name

  set(TEST_NAME unittests_${ARG_TARGET})

  # Sources

  # Join target and test sources
  get_target_property(TARGET_SOURCES ${ARG_TARGET} SOURCES)

  get_target_property(TARGET_SOURCE_DIR ${ARG_TARGET} SOURCE_DIR)
  foreach(DIR ${TARGET_SOURCES})
    get_filename_component(
      DIR
      ${DIR}
      ABSOLUTE
      BASE_DIR
      ${TARGET_SOURCE_DIR}
    )
    list(APPEND ARG_SOURCES ${DIR})
  endforeach()

  add_executable(${TEST_NAME} ${ARG_SOURCES})

  add_test(${TEST_NAME} ${TEST_NAME})

  # Includes

  # Join target and test includes
  get_target_property(TARGET_INCLUDES ${ARG_TARGET} INCLUDE_DIRECTORIES)
  list(APPEND TARGET_INCLUDES ${ARG_INCLUDES})

  if(TARGET_INCLUDES)
    target_include_directories(${TEST_NAME} PRIVATE ${TARGET_INCLUDES})
  endif()

  # Libraries

  # Join target and test libraries
  get_target_property(TARGET_LINKS ${ARG_TARGET} LINK_LIBRARIES)
  if(TARGET_LINKS STREQUAL TARGET_LINKS-NOTFOUND)
    set(TARGET_LINKS)
  endif()
  list(
    APPEND
    TARGET_LINKS
    ${ARG_LINKS}
    gtest
    gmock
  )

  # Add coverage facilities if `lcov` specified in the tool list.
  set(LCOV_EXE lcov)

  if((${LCOV_EXE} IN_LIST TOOLS))
    list(APPEND TARGET_LINKS gcov)
    # Compiler
    target_compile_options(${TEST_NAME} PRIVATE --coverage)
  endif()

  target_link_libraries(${TEST_NAME} PRIVATE ${TARGET_LINKS})

  # Run unittests and make lcov report

  if(ARG_DO_NOT_RUN)
    return()
  endif()

  get_target_property(TARGET_BINARY_DIR ${TEST_NAME} BINARY_DIR)
  set(REPORT_DIR ${CMAKE_SOURCE_DIR}/reports/lcov)
  set(LCOV_INFO ${REPORT_DIR}/lcov.info)
  string(TIMESTAMP TIME_NOW "%y%m%d_%H%M%S")
  set(RESULT_DIR ${REPORT_DIR}/${ARG_TARGET}_${TIME_NOW})

  add_custom_command(
    TARGET ${TEST_NAME}
    POST_BUILD
    COMMAND ${TARGET_BINARY_DIR}/${TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running ${TEST_NAME}"
    VERBATIM
  )

  # Check lcov existence and run coverage analysis

  if(NOT (${LCOV_EXE} IN_LIST TOOLS))
    return()
  endif()

  find_program(TOOL_FOUND ${LCOV_EXE} NO_CACHE)
  if(NOT TOOL_FOUND)
    message(
      YELLOW
      "'${LCOV_EXE}' tool not found.\nInstall '${LCOV_EXE}' in order to use '${CMAKE_CURRENT_FUNCTION}' function. The report will not be generated."
    )
  else()
    add_custom_command(
      TARGET ${TEST_NAME}
      POST_BUILD
      COMMAND mkdir -p ${REPORT_DIR}
      COMMAND ${LCOV_EXE} -c -d ${TARGET_BINARY_DIR} -o ${LCOV_INFO} --exclude "/usr/include/*" --exclude "/usr/local/*"
              --exclude "*/tests/*" > /dev/null 2>&1
      COMMAND genhtml ${LCOV_INFO} -o ${RESULT_DIR} > /dev/null
      COMMAND rm ${LCOV_INFO}
      COMMAND
        echo
        "\\e[30m-Report generated in directory \\e[37m${RESULT_DIR}\\e[30m. Open \\e[37mindex.html\\e[30m in browser.\\e[m"
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      VERBATIM
    )
  endif()

endfunction()
