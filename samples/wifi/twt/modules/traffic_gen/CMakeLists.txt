#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(inc)

target_sources_ifdef(CONFIG_TRAFFIC_GEN app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_TRAFFIC_GEN app PRIVATE src/tcp.c)
