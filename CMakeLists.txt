#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Point to NCS root directory.
set(NRF_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "NCS root directory")

# We need to cache the build type so that it is not out of sync when CONF_FILE
# is cached.
set(CONF_FILE_BUILD_TYPE ${CONF_FILE_BUILD_TYPE} CACHE INTERNAL "The build type")

foreach(runner_ext BIN HEX ELF)
  zephyr_get(NCS_RUNNER_${runner_ext} SYSBUILD)
  if(DEFINED NCS_RUNNER_${runner_ext})
    string(TOLOWER "${runner_ext}" type)
    set_target_properties(runners_yaml_props_target PROPERTIES
                          "${type}_file" "${NCS_RUNNER_${runner_ext}}"
    )
  endif()
endforeach()

include(cmake/extensions.cmake)
include(cmake/version.cmake)
include(cmake/version_app.cmake)
include(cmake/multi_image.cmake)

zephyr_include_directories(include)

add_subdirectory(ext)
add_subdirectory(lib)
add_subdirectory(samples)
add_subdirectory(subsys)
add_subdirectory(modules)
add_subdirectory(drivers)
add_subdirectory(tests)
