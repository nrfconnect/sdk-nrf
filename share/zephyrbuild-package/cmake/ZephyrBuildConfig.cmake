# Relative directory of NRF dir as seen from ZephyrExtension package file
set(NRF_RELATIVE_DIR "../../..")

# Set the current NRF_DIR
# The use of get_filename_component ensures that the final path variable will not contain `../..`.
get_filename_component(NRF_DIR ${CMAKE_CURRENT_LIST_DIR}/${NRF_RELATIVE_DIR} ABSOLUTE)

include(${NRF_DIR}/cmake/boilerplate.cmake NO_POLICY_SCOPE)
