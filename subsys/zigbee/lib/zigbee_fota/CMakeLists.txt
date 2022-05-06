#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(
  src/zigbee_fota.c
  src/dfu_multi_target.c
  )

zephyr_library_link_libraries(zboss)
zephyr_library_link_libraries(zigbee)
