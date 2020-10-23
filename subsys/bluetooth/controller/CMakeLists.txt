#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
  hci_driver.c
  hci_internal.c
)

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_CRYPTO
  crypto.c
)

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_ECDH
  ecdh.c
)

zephyr_library_link_libraries_ifdef(
  CONFIG_BT_CTLR_ECDH_LIB_OBERON
  nrfxlib_crypto
)

zephyr_library_link_libraries(subsys__bluetooth)

zephyr_include_directories(.)
