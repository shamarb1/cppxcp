if(bug-finder IN_LIST TOOLS OR code-prover IN_LIST TOOLS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

function(polyspace_analysis)

  # Parse arguments

  set(OPTIONS BUG_FINDER CODE_PROVER)
  set(ONE_VALUE_ARGS POLYSPACE_INSTALLATION_DIR)
  cmake_parse_arguments(
    ARG
    "${OPTIONS}"
    "${ONE_VALUE_ARGS}"
    ""
    ${ARGN}
  )

  if(NOT (bug-finder IN_LIST TOOLS))
    set(ARG_BUG_FINDER OFF)
  endif()
  if(NOT (code-prover IN_LIST TOOLS))
    set(ARG_CODE_PROVER OFF)
  endif()

  # Check arguments for correctness

  if(NOT ARG_BUG_FINDER AND NOT ARG_CODE_PROVER)
    message(YELLOW "Skipping Polyspace analysis.")
    return()
  endif()

  if(NOT ARG_POLYSPACE_INSTALLATION_DIR)
    set(ARG_POLYSPACE_INSTALLATION_DIR "/usr/local/Polyspace/R2022b")
  endif()

  find_program(TMP ${ARG_POLYSPACE_INSTALLATION_DIR}/polyspace/bin/polyspace-configure NO_CACHE)
  if(NOT TMP)
    message(
      YELLOW
      "Polyspace tools not found on path \"${ARG_POLYSPACE_INSTALLATION_DIR}\".\nInstall Polyspace in order to use '${CMAKE_CURRENT_FUNCTION}' function. Skipping analysis."
    )
    return()
  endif()

  # Set variables

  set(POLYSPACE_BIN_DIR "${ARG_POLYSPACE_INSTALLATION_DIR}/polyspace/bin")
  set(POLYSPACE_TEMPLATE_DIR "${ARG_POLYSPACE_INSTALLATION_DIR}/toolbox/polyspace/psrptgen/templates")
  set(REPORT_DIR "${CMAKE_SOURCE_DIR}/reports/polyspace")
  set(POLYSPACE_TMP_DIR "${REPORT_DIR}/tmp")
  set(POLYSPACE_TMP_BUG_FINDER_DIR "${POLYSPACE_TMP_DIR}/bug_finder")
  set(POLYSPACE_TMP_CODE_PROVER_DIR "${POLYSPACE_TMP_DIR}/code_prover")
  set(POLYSPACE_OPTIONS "${POLYSPACE_TMP_DIR}/polyspace_options.txt")
  string(TIMESTAMP TIME_NOW "%y%m%d_%H%M%S")
  set(BUG_FINDER_REPORT_FILE "${REPORT_DIR}/bug_finder_${TIME_NOW}")
  set(CODE_PROVER_REPORT_FILE "${REPORT_DIR}/code_prover_${TIME_NOW}")
  cmake_host_system_information(RESULT NUMBER_OF_CPU QUERY NUMBER_OF_PHYSICAL_CORES)

  # Polyspace uses `compile_commands.json`. This file is created after configuration is done. It means that it's not
  # possible to run the script during CMake generation steps. Therefore, create a distinct target and run commands
  # during the build steps.
  add_custom_target(polyspace ALL)

  # Generate options file

  add_custom_command(
    TARGET polyspace
    COMMAND ${POLYSPACE_BIN_DIR}/polyspace-configure -allow-overwrite -compilation-database
            ${CMAKE_BINARY_DIR}/compile_commands.json -output-options-file ${POLYSPACE_OPTIONS} > /dev/null VERBATIM
  )

  # Run Bug Finder

  if(ARG_BUG_FINDER)
    add_custom_command(
      TARGET polyspace
      COMMAND
        ${POLYSPACE_BIN_DIR}/polyspace-bug-finder -options-file ${POLYSPACE_OPTIONS} -results-dir
        ${POLYSPACE_TMP_BUG_FINDER_DIR} -import-comments ${POLYSPACE_TMP_BUG_FINDER_DIR} -do-not-generate-results-for
        all-headers -autosar-cpp14 all -misra3 mandatory-required -checkers all -code-metrics -max-processes
        ${NUMBER_OF_CPU} > /dev/null && ${POLYSPACE_BIN_DIR}/polyspace-report-generator -format HTML -template
        ${POLYSPACE_TEMPLATE_DIR}/bug_finder/BugFinder.rpt -results-dir ${POLYSPACE_TMP_BUG_FINDER_DIR} -output-name
        ${BUG_FINDER_REPORT_FILE} > /dev/null && echo
        "\\e[30m-Report generated \\e[37m${BUG_FINDER_REPORT_FILE}.html\\e[30m.\\e[m" || echo
        "\\e[33mPolyspace Bug Finder analysis failed. Check you are in Volvo intranet or VPN is connected.\\e[m" && exit
        0
      COMMENT "Running Polyspace Bug Finder analysis."
      VERBATIM
    )
  endif()

  # Run Code Prover

  if(ARG_CODE_PROVER)
    add_custom_command(
      TARGET polyspace
      COMMAND
        ${POLYSPACE_BIN_DIR}/polyspace-code-prover -options-file ${POLYSPACE_OPTIONS} -results-dir
        ${POLYSPACE_TMP_CODE_PROVER_DIR} -do-not-generate-results-for all-headers -to pass2 -O2
        -unsigned-integer-overflows allow -signed-integer-overflows allow -uncalled-function-checks all
        -detect-pointer-escape -main-generator -class-analyzer all -max-processes ${NUMBER_OF_CPU} > /dev/null &&
        ${POLYSPACE_BIN_DIR}/polyspace-report-generator -format HTML -template ${POLYSPACE_TEMPLATE_DIR}/Developer.rpt
        -results-dir ${POLYSPACE_TMP_CODE_PROVER_DIR} -output-name ${CODE_PROVER_REPORT_FILE} > /dev/null && echo
        "\\e[30m-Report generated \\e[37m${CODE_PROVER_REPORT_FILE}.html\\e[30m.\\e[m" || echo
        "\\e[33mPolyspace Code Prover analysis failed. Check you are in Volvo intranet or VPN is connected.\\e[m" &&
        exit 0
      COMMENT "Running Polyspace Code Prover analysis."
      VERBATIM
    )
  endif()

  # Clean up

  add_custom_command(TARGET polyspace COMMAND rm -rf ${POLYSPACE_TMP_DIR} VERBATIM)

endfunction()
