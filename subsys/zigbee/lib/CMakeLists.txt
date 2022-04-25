#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_ZIGBEE_FOTA zigbee_fota)
add_subdirectory_ifdef(CONFIG_ZIGBEE_APP_UTILS zigbee_app_utils)
add_subdirectory_ifdef(CONFIG_ZIGBEE_LOGGER_EP zigbee_logger_ep)
add_subdirectory_ifdef(CONFIG_ZIGBEE_SCENES zigbee_scenes)
add_subdirectory_ifdef(CONFIG_ZIGBEE_SHELL zigbee_shell)
