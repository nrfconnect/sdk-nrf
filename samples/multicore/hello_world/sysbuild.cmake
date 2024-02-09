#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add remote project
ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_REMOTE_BOARD}
  )

if(NOT "${SB_CONFIG_REMOTE_IMAGE_DOMAIN}" IN_LIST PM_DOMAINS)
  set_property(GLOBAL APPEND PROPERTY PM_DOMAINS ${SB_CONFIG_REMOTE_IMAGE_DOMAIN})
endif()

set_property(GLOBAL APPEND PROPERTY PM_${SB_CONFIG_REMOTE_IMAGE_DOMAIN}_IMAGES remote)
set_property(GLOBAL PROPERTY DOMAIN_APP_${SB_CONFIG_REMOTE_IMAGE_DOMAIN} remote)
set(${SB_CONFIG_REMOTE_IMAGE_DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION remote CACHE INTERNAL "")

# Add a dependency so that the remote sample will be built and flashed first
add_dependencies(hello_world remote)
# Add dependency so that the remote image is flashed first.
sysbuild_add_dependencies(FLASH hello_world remote)
