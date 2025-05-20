#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_NFC_RPC_T2T nfc_rpc_t2t_client.c)
zephyr_library_sources_ifdef(CONFIG_NFC_RPC_T4T nfc_rpc_t4t_client.c)

zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nfc/include/)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
)
