#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_HAS_HW_NRF_KMU)
  message(FATAL_ERROR "No KMU support added.")
else()
  set(PROVISION_SRC provision_flash.c)
endif()

zephyr_library()
zephyr_library_sources(${PROVISION_SRC})
