#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE main.c)
target_sources_ifdef(CONFIG_LTE_LINK_CONTROL app PRIVATE network_lte.c)
target_sources_ifdef(CONFIG_WIFI app PRIVATE network_wifi.c)
target_include_directories(app PRIVATE .)
