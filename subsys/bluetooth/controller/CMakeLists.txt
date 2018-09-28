#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library()

zephyr_library_sources(
  crypto.c
  hci_driver.c
  )

zephyr_library_link_libraries(subsys__bluetooth)
