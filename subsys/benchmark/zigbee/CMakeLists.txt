#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(
  zigbee_benchmark.c
  zigbee_protocol.c
  zigbee_timer.c
  zigbee_zcl_commands.c
  zigbee_commands.c
)

zephyr_library_link_libraries(zboss)
