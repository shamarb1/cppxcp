function(target_clang_format ARG_TARGET)

  if(NOT (clang-format IN_LIST TOOLS))
    return()
  endif()

  # Check tool existence

  set(TOOL_EXE clang-format)

  find_program(TMP ${TOOL_EXE} NO_CACHE)
  if(NOT TMP)
    message(
      YELLOW
      "'${TOOL_EXE}' tool not found.\nInstall '${TOOL_EXE}' in order to use '${CMAKE_CURRENT_FUNCTION}' function. Skipping '${ARG_TARGET}` formatting."
    )
    return()
  endif()

  # Set variables

  get_target_property(TARGET_SOURCE_DIR ${ARG_TARGET} SOURCE_DIR)

  # Run clang formatting

  # cmake-format: off
  execute_process(
    COMMAND find -type f -regextype posix-extended -regex ".*\\\.(c|h|cpp|hpp)" -exec ${TOOL_EXE} -i {} \;
    WORKING_DIRECTORY ${TARGET_SOURCE_DIR} OUTPUT_VARIABLE OUT
  )
  # cmake-format: on

  if(OUT)
    message(YELLOW ${OUT})
  else()
    message(BLUE_BOLD "'${ARG_TARGET}' formated with ${TOOL_EXE}.")
  endif()

endfunction()
