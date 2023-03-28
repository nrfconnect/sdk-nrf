#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This check is needed to avoid infinite recursion. This module code will
# be executed for all images in the build, also for the child image being
# added below.
if (CONFIG_APP_INCLUDE_REMOTE_IMAGE)
  add_child_image(
    NAME remote
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../remote
    DOMAIN remote
    BOARD ${CONFIG_APP_REMOTE_BOARD}
    )
endif()
