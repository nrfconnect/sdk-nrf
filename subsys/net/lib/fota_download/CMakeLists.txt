#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(
  src/fota_download.c
  )

zephyr_include_directories_ifdef(CONFIG_SECURE_BOOT
  ${ZEPHYR_BASE}/../nrf/subsys/dfu/include)
