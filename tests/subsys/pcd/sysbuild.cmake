#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  message(FATAL_ERROR "REMOTE_BOARD must be set to a valid board name")
endif()

# Add remote project
ExternalZephyrProject_Add(
    APPLICATION hello_world
    SOURCE_DIR ${ZEPHYR_BASE}/samples/hello_world
    BOARD ${SB_CONFIG_REMOTE_BOARD}
  )

set_property(GLOBAL APPEND PROPERTY PM_CPUNET_IMAGES hello_world)
set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET hello_world)
set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION hello_world CACHE INTERNAL "")

# Add a dependency so that the remote sample will be built and flashed first
add_dependencies(pcd hello_world)
# Add dependency so that the remote image is flashed first.
sysbuild_add_dependencies(FLASH pcd hello_world)
