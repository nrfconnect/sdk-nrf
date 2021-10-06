# The ZephyrBuildConfig is simply including NCS package.
# The NCS package can be used to precisely identify an NCS installation, in case there are multiple Zephyr derivatives installed.

# Only source the NcsConfig file when `find_package(Ncs)` has not been used.
if(NOT DEFINED Ncs_DIR)
  include(${CMAKE_CURRENT_LIST_DIR}/../../ncs-package/cmake/NcsConfig.cmake)
endif()
