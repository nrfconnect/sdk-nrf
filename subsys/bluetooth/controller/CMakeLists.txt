#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library()

zephyr_library_sources(
  hci_driver.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_CRYPTO
  crypto.c
)

zephyr_library_link_libraries(subsys__bluetooth)

zephyr_include_directories(.)
