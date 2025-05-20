#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory(lib)
add_subdirectory_ifdef(CONFIG_OPENTHREAD_RPC openthread/rpc)
add_subdirectory_ifdef(CONFIG_L2_WIFI_CONNECTIVITY l2_wifi_if_conn)
add_subdirectory_ifdef(CONFIG_OPENTHREAD_REPORT openthread/report)
add_subdirectory_ifdef(CONFIG_NET_L2_OPENTHREAD openthread/settings_check)
