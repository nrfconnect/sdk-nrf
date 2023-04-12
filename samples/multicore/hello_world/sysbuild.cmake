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

# Add a dependency so that the remote sample will be built and flashed first
add_dependencies(hello_world remote)
# Place remote image first in the image list
set(IMAGES "remote" ${IMAGES})
