#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(
  src/fota_download.c
  src/fota_download_util.c
)
zephyr_include_directories(./include)
zephyr_include_directories_ifdef(CONFIG_SECURE_BOOT
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/include)
