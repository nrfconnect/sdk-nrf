#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(zigbee_zcl_scenes.c)

zephyr_library_link_libraries(zboss)
zephyr_library_link_libraries(zigbee)
