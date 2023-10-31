if(clang-tidy IN_LIST TOOLS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

function(target_clang_tidy ARG_TARGET)

  if(NOT (clang-tidy IN_LIST TOOLS))
    return()
  endif()

  # Check tool existence

  set(TOOL_EXE clang-tidy)

  find_program(TMP ${TOOL_EXE} NO_CACHE)
  if(NOT TMP)
    message(
      YELLOW
      "'${TOOL_EXE}' tool not found.\nInstall '${TOOL_EXE}' in order to use '${CMAKE_CURRENT_FUNCTION}' function. Skipping '${ARG_TARGET}` analysis."
    )
    return()
  endif()

  # Set variables

  get_target_property(TARGET_SOURCE_DIR ${ARG_TARGET} SOURCE_DIR)
  string(TIMESTAMP TIME_NOW "%y%m%d_%H%M%S")
  set(REPORT_DIR "${CMAKE_SOURCE_DIR}/reports/${TOOL_EXE}")
  set(REPORT_FILE "${REPORT_DIR}/${ARG_TARGET}_${TIME_NOW}.log")
  set(CLANG_TIDY_TARGET "clang_tidy_${ARG_TARGET}")

  # clang-tidy uses `compile_commands.json`. This file is created after configuration is done. It means that it's not
  # possible to run the script during CMake generation steps. Therefore, create a distinct target and run commands
  # during the build steps.
  add_custom_target(${CLANG_TIDY_TARGET})

  add_dependencies(${ARG_TARGET} ${CLANG_TIDY_TARGET})

  # Collect include directories

  get_target_property(TARGET_INCLUDES ${ARG_TARGET} INCLUDE_DIRECTORIES)
  foreach(INCLUDE ${TARGET_INCLUDES} /usr/include/c++/9 /usr/include/x86_64-linux-gnu/c++/9)
    list(APPEND INCLUDE_LIST "-I${INCLUDE}")
  endforeach()

  # Run clang-tidy analysis

  add_custom_command(
    TARGET ${CLANG_TIDY_TARGET}
    COMMAND mkdir -p ${REPORT_DIR}
    COMMAND find -type f -regextype posix-extended -regex ".*\.(c|h|cpp|hpp)" -exec ${TOOL_EXE} -p ${CMAKE_BINARY_DIR}
            {} -- ${INCLUDE_LIST} \; > ${REPORT_FILE} 2>&1
    COMMAND echo "\\e[30m-Report generated \\e[37m${REPORT_FILE}\\e[30m.\\e[m"
    WORKING_DIRECTORY ${TARGET_SOURCE_DIR}
    COMMENT "Running ${TOOL_EXE} analysis on '${ARG_TARGET}'."
    VERBATIM
  )

endfunction()
