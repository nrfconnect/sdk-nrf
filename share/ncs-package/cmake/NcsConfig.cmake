# Relative directory of NRF dir as seen from ZephyrExtension package file
set(NRF_RELATIVE_DIR "../../..")
set(NCS_RELATIVE_DIR "../../../..")

# Set the current NRF_DIR
# The use of get_filename_component ensures that the final path variable will not contain `../..`.
get_filename_component(NRF_DIR ${CMAKE_CURRENT_LIST_DIR}/${NRF_RELATIVE_DIR} ABSOLUTE)
get_filename_component(NCS_DIR ${CMAKE_CURRENT_LIST_DIR}/${NCS_RELATIVE_DIR} ABSOLUTE)

file(STRINGS ${NRF_DIR}/VERSION NCS_VERSION LIMIT_COUNT 1 LENGTH_MINIMUM 5)
string(REGEX MATCH "([^\.]*)\.([^\.]*)\.([^-]*)[-]?(.*)" OUT_VAR ${NCS_VERSION})

set(NCS_VERSION_MAJOR ${CMAKE_MATCH_1})
set(NCS_VERSION_MINOR ${CMAKE_MATCH_2})
set(NCS_VERSION_PATCH ${CMAKE_MATCH_3})
set(NCS_VERSION_EXTRA ${CMAKE_MATCH_4})

if(NOT NO_BOILERPLATE)
  include(${NRF_DIR}/cmake/boilerplate.cmake NO_POLICY_SCOPE)
endif()
