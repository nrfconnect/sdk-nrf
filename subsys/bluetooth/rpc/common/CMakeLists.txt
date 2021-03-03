#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_link_libraries(subsys_bluetooth_rpc)

zephyr_library_sources(bt_rpc_common.c
                       cbkproxy.c
                       serialize.c)
