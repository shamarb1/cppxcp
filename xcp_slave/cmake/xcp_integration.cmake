function(target_link_library_xcp ARG_TARGET)

  # Parse arguments

  set(OPTIONS CALIBRATION A2L_GENERATION)
  set(ONE_VALUE_ARGS
      UDP_PORT
      BASE_DIR
      INCLUDE_DIR
      LIB
  )
  cmake_parse_arguments(
    ARG
    "${OPTIONS}"
    "${ONE_VALUE_ARGS}"
    ""
    ${ARGN}
  )

  # Check arguments for correctness

  if(NOT (ARG_UDP_PORT MATCHES "^[0-9]+$") OR (ARG_UDP_PORT LESS 30000) OR (ARG_UDP_PORT GREATER 30489))
    message(FATAL_ERROR "UDP_PORT shall be an integer in range [30000..30489].")
  endif()

  if(NOT ARG_BASE_DIR)
    set(ARG_BASE_DIR "${XCP_SLAVE_BASE_DIR}")
  endif()

  if(NOT ARG_INCLUDE_DIR)
    set(ARG_INCLUDE_DIR "${XCP_SLAVE_INCLUDE_DIR}")
  endif()

  if(NOT ARG_LIB)
    set(ARG_LIB "${XCP_SLAVE_LIBXCP_SLAVE}")
  endif()

  # Import library

  set(XCP_SLAVE_TARGET_NAME "xcp_slave_lib")
  add_library(${XCP_SLAVE_TARGET_NAME} SHARED IMPORTED)
  set_target_properties(${XCP_SLAVE_TARGET_NAME} PROPERTIES IMPORTED_LOCATION "${ARG_LIB}")
  target_include_directories(${XCP_SLAVE_TARGET_NAME} INTERFACE ${ARG_INCLUDE_DIR})
  target_link_libraries(${ARG_TARGET} PRIVATE ${XCP_SLAVE_TARGET_NAME})

  set(STATUS_MESSAGE "XCP library linked successfully")

  # Set UDP port

  set(STATUS_MESSAGE "${STATUS_MESSAGE} on UDP port ${ARG_UDP_PORT} with")
  target_compile_definitions(${ARG_TARGET} PRIVATE XCP_SLAVE_UDP_PORT=${ARG_UDP_PORT})

  # Set calibration support

  if(ARG_CALIBRATION)
    target_compile_definitions(${ARG_TARGET} PRIVATE XCP_SLAVE_CALIBRATION_SUPPORT)
  else()
    set(STATUS_MESSAGE "${STATUS_MESSAGE}out")
  endif()
  set(STATUS_MESSAGE "${STATUS_MESSAGE} calibration and with")

  # Set tools directory

  set(TOOLS_DIR ${ARG_BASE_DIR}/tools)

  # Pass script to the linker

  target_link_options(${ARG_TARGET} PRIVATE -T${TOOLS_DIR}/linker_scripts/sections.ld)

  # Generate A2L and S19 files

  if(ARG_A2L_GENERATION)

    # Check compiler version.
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 11.0.0)
      message(WARNING "GCC version >=11. Recommended GCC version is 9. A2L generation may not be successful.")
    endif()

    # Check for `-g` compiler flag.
    get_target_compiler_flags(${ARG_TARGET} COMPILER_FLAGS)
    if("-g" IN_LIST COMPILER_FLAGS)
      generate_a2l_s19(
        ${ARG_TARGET}
        ${ARG_UDP_PORT}
        ${ARG_CALIBRATION}
        ${TOOLS_DIR}
      )
    else()
      message(
        WARNING
          "A2L ans S19 file will not be generated though the A2L_GENERATION flag has been passed. Output binary file shall contain debug information. Use a compiler flag -g."
      )
      set(STATUS_MESSAGE "${STATUS_MESSAGE}out")
    endif()

  else()
    set(STATUS_MESSAGE "${STATUS_MESSAGE}out")
  endif()

  string(ASCII 27 ESC)
  message("${ESC}[1;34m${STATUS_MESSAGE} A2L generation.${ESC}[m") # Blue color text

endfunction()

function(
  generate_a2l_s19
  ARG_TARGET
  ARG_UDP_PORT
  ARG_CALIBRATION
  ARG_TOOLS_DIR
)

  # Collect all source files used in target

  get_target_property(TARGET_SOURCE_DIR ${ARG_TARGET} SOURCE_DIR)
  get_target_property(TARGET_SOURCES ${ARG_TARGET} SOURCES)
  foreach(SOURCE ${TARGET_SOURCES})
    # Convert to absolute path
    get_filename_component(
      SOURCE
      ${SOURCE}
      ABSOLUTE
      BASE_DIR
      ${TARGET_SOURCE_DIR}
    )
    # Append to argument list
    list(
      APPEND
      SOURCES
      "-source-path"
      "${SOURCE}"
    )
  endforeach()

  # Set variables

  set(A2L_EXE "tmp_raw_${ARG_TARGET}.a2l")
  set(A2L_PORT "tmp_port_changed_template.a2l")
  set(A2L_ADDRESS "tmp_address_updated_template.a2l")

  set(VCC_A2L_TOOLS_DIR ${ARG_TOOLS_DIR}/vcc_a2l_tools)
  set(A2L_TEMPLATES_DIR ${ARG_TOOLS_DIR}/a2l_templates)

  if(ARG_CALIBRATION)
    set(A2L_TEMPLATES_DIR ${A2L_TEMPLATES_DIR}/calibration)
  endif()

  if(${CMAKE_SYSTEM_NAME} STREQUAL "QNX")
    set(A2L_TEMPLATE ${A2L_TEMPLATES_DIR}/hpa.a2l)
  else()
    set(A2L_TEMPLATE ${A2L_TEMPLATES_DIR}/vhpa.a2l)
  endif()

  get_target_property(TARGET_BINARY_DIR ${ARG_TARGET} BINARY_DIR)

  # Run commands for a2l and s19 generation

  add_custom_command(
    TARGET ${ARG_TARGET}
    POST_BUILD
    COMMAND ${VCC_A2L_TOOLS_DIR}/VccA2lCreator -e ${ARG_TARGET} ${SOURCES} -o ${A2L_EXE}
    COMMAND sed -e "s/DUMMY_PORT_VALUE/${ARG_UDP_PORT}/g" ${A2L_TEMPLATE} > ${A2L_PORT}
    COMMAND ${VCC_A2L_TOOLS_DIR}/VccA2lAddressUpdate -i ${A2L_PORT} -e ${ARG_TARGET} -o ${A2L_ADDRESS}
            -update-memory-segments
    COMMAND ${VCC_A2L_TOOLS_DIR}/VccA2lMerge -m ${A2L_ADDRESS} -i ${A2L_EXE} -o ${ARG_TARGET}.a2l
    COMMAND echo "\\e[30m-A2L file generated \\e[37m${TARGET_BINARY_DIR}/${ARG_TARGET}.a2l\\e[30m.\\e[m"
    COMMAND objcopy -O srec --srec-forceS3 ${ARG_TARGET} ${ARG_TARGET}.s19
    COMMAND echo "\\e[30m-S19 file generated \\e[37m${TARGET_BINARY_DIR}/${ARG_TARGET}.s19\\e[30m.\\e[m"
    COMMAND rm ${A2L_EXE} ${A2L_PORT} ${A2L_ADDRESS}
    WORKING_DIRECTORY ${TARGET_BINARY_DIR}
    COMMENT "Generating A2L and S19 files."
    VERBATIM
  )

endfunction()

function(get_target_compiler_flags ARG_TARGET ARG_COMPILER_FLAGS)

  get_target_property(VALUE ${ARG_TARGET} COMPILE_OPTIONS)
  if(VALUE)
    list(APPEND ARG_COMPILER_FLAGS ${VALUE})
  endif()

  string(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" VALUE)
  set(VALUE "${${VALUE}}")
  if(VALUE)
    list(APPEND ARG_COMPILER_FLAGS ${VALUE})
  endif()

  list(APPEND ARG_COMPILER_FLAGS ${CMAKE_CXX_FLAGS})

  separate_arguments(ARG_COMPILER_FLAGS)

  set(${ARG_COMPILER_FLAGS} PARENT_SCOPE)

endfunction()
