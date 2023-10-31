define_property(
  GLOBAL
  PROPERTY ENABLED_PROJECTS
  BRIEF_DOCS "List of enabled targets."
  FULL_DOCS "List of enabled targets added in the directory by calls to the add_project() command."
)

function(add_project ARG_PROJECT)

  # Check arguments for correctness

  if(NOT ARG_PROJECT)
    message(FATAL_ERROR "Project name should not be empty.")
  endif()

  if(NOT ARGV1)
    set(ARG_DIR ${ARG_PROJECT})
  else()
    set(ARG_DIR ${ARGV1})
  endif()

  # Convert to absolute path
  get_filename_component(
    ARG_DIR
    ${ARG_DIR}
    ABSOLUTE
    BASE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
  )

  if(NOT EXISTS ${ARG_DIR})
    message(FATAL_ERROR "Directory `${ARG_DIR}` does not exist.")
  endif()

  # Set variables

  if(${ARG_PROJECT} IN_LIST PROJECTS)
    set_property(GLOBAL APPEND PROPERTY ENABLED_PROJECTS "${ARG_PROJECT}")
    add_subdirectory(${ARG_DIR})
  endif()

endfunction()
