function(cmake_format)

  if(NOT (cmake-format IN_LIST TOOLS))
    return()
  endif()

  # Set variables

  set(TOOL_EXE cmake-format)

  # Check tool existence

  find_program(TMP ${TOOL_EXE} NO_CACHE)
  if(NOT TMP)
    message(
      YELLOW
      "'${TOOL_EXE}' tool not found.\nInstall '${TOOL_EXE}' in order to use '${CMAKE_CURRENT_FUNCTION}' function. Skipping CMake files formatting."
    )
    return()
  endif()

  # Run cmake formatting

  # cmake-format: off
  execute_process(
    COMMAND find ! -path "./build*" \( -name "*.cmake" -o -name "CMakeLists.txt" \) -exec ${TOOL_EXE} -i {} \;
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE OUT
  )
  # cmake-format: on

  if(OUT)
    message(YELLOW ${OUT})
  else()
    message(BLUE_BOLD "CMake files formated.")
  endif()

endfunction()
