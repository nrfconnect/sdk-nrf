#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_BSD_LIBRARY bsdlib)
add_subdirectory_ifdef(CONFIG_LWM2M_CARRIER lwm2m_carrier)
add_subdirectory_ifdef(CONFIG_DK_LIBRARY dk_buttons_and_leds)
add_subdirectory_ifdef(CONFIG_AT_HOST_LIBRARY at_host)
add_subdirectory_ifdef(CONFIG_AT_CMD_PARSER at_cmd_parser)
add_subdirectory_ifdef(CONFIG_MODEM_INFO modem_info)
add_subdirectory_ifdef(CONFIG_PDN_MANAGEMENT pdn_management)
