#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add child image only while construction main image
if (CONFIG_APP_INCLUDE_REMOTE_IMAGE)
  add_child_image(
    NAME remote
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../remote
    DOMAIN remote
    BOARD ${CONFIG_APP_REMOTE_BOARD}
    )
endif()
