#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(
  src/fota_download.c
  src/util/fota_download_util.c
)

zephyr_library_sources_ifdef(CONFIG_DFU_TARGET_MCUBOOT
  src/util/fota_download_mcuboot.c
)

zephyr_library_sources_ifdef(CONFIG_DFU_TARGET_FULL_MODEM
  src/util/fota_download_full_modem.c
)

zephyr_library_sources_ifdef(CONFIG_DFU_TARGET_MODEM_DELTA
  src/util/fota_download_delta_modem.c
)

zephyr_library_sources_ifdef(CONFIG_DFU_TARGET_SMP
  src/util/fota_download_smp.c
)

zephyr_include_directories(./include)
zephyr_include_directories_ifdef(CONFIG_SECURE_BOOT
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/include)
zephyr_include_directories_ifdef(CONFIG_DOWNLOAD_CLIENT
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/download_client/include)
