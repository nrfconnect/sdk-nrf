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

# Customize the Zephyr kernel version.h using nRF Connect SDK ncs_version.h.
set(KERNEL_VERSION_CUSTOMIZATION \#include;<ncs_version.h> PARENT_SCOPE)

include(cmake/extensions.cmake)
include(cmake/version.cmake)
include(cmake/multi_image.cmake)

zephyr_include_directories(include)

add_subdirectory(ext)
add_subdirectory(lib)
add_subdirectory(samples)
add_subdirectory(subsys)
add_subdirectory(modules)
add_subdirectory(drivers)
add_subdirectory(tests)
