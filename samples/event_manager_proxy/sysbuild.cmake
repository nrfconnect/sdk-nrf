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
list(APPEND PM_DOMAINS CPUNET)
set(PM_CPUNET_IMAGES remote)
set(DOMAIN_APP_CPUNET remote)
set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION remote CACHE INTERNAL "")

# Add a dependency so that the remote sample will be built and flashed first
add_dependencies(event_manager_proxy remote)
# Place remote image first in the image list
set(IMAGES "remote" ${IMAGES})
