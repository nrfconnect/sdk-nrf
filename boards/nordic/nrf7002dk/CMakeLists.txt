# Copyright (c) 2022 Nordic Semiconductor ASA.
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_BOARD_ENABLE_CPUNET)
  if(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP OR CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP_NS)
    zephyr_library()
    zephyr_library_sources(nrf5340_cpunet_reset.c)
  elseif(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP_NRF7001 OR CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP_NS_NRF7001)
    zephyr_library()
    zephyr_library_sources(nrf5340_cpunet_nrf7001_reset.c)
  endif()
endif()
