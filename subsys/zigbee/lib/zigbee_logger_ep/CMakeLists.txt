#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library()
zephyr_library_sources(zigbee_logger_eprxzcl.c)

zephyr_library_link_libraries(zboss)
zephyr_library_link_libraries(zigbee)
