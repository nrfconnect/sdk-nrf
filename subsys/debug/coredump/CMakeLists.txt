#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_DEBUG_COREDUMP_BACKEND_NRF_FLASH_PARTITION OR CONFIG_DEBUG_COREDUMP_BACKEND_NRF_RAM_REGION)
  zephyr_sources(coredump_backend_nrf.c)
endif()
