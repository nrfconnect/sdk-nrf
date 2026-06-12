# Relative directory of NRF dir as seen from ZephyrExtension package file
set(NRF_RELATIVE_DIR "../../..")
set(NCS_RELATIVE_DIR "../../../..")

# Set the current NRF_DIR
# The use of get_filename_component ensures that the final path variable will not contain `../..`.
get_filename_component(NRF_DIR ${CMAKE_CURRENT_LIST_DIR}/${NRF_RELATIVE_DIR} ABSOLUTE)
get_filename_component(NCS_DIR ${CMAKE_CURRENT_LIST_DIR}/${NCS_RELATIVE_DIR} ABSOLUTE)

if(NOT NO_BOILERPLATE)
  include(${NRF_DIR}/cmake/boilerplate.cmake NO_POLICY_SCOPE)
endif()

set(VERSION_TYPE NCS)
set(VERSION_FILE ${NRF_DIR}/VERSION)
include(${ZEPHYR_BASE}/cmake/modules/version.cmake)

foreach(type file IN ZIP_LISTS VERSION_TYPE VERSION_FILE)
  if(NOT EXISTS ${file})
    break()
  endif()
  file(READ ${file} ver)

  string(REGEX MATCH "VERSION_METADATA = ([a-z0-9\.\-]*)" _ ${ver})
  set(${type}_VERSION_METADATA ${CMAKE_MATCH_1})
endforeach()

set(VERSION_TYPE)
set(VERSION_FILE)
