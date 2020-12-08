#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_ZIGBEE_FOTA zigbee_fota)
add_subdirectory_ifdef(CONFIG_ZIGBEE_APP_UTILS zigbee_app_utils)
add_subdirectory_ifdef(CONFIG_ZIGBEE_LOGGER_EP zigbee_logger_ep)
