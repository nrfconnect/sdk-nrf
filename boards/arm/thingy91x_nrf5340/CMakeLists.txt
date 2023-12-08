# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if (CONFIG_BOARD_THINGY91X_NRF5340_CPUAPP OR CONFIG_BOARD_THINGY91X_NRF5340_CPUAPP_NS)
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_BOARD_ENABLE_CPUNET nrf5340_cpunet_reset.c)
set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf5340_pm_static.yml CACHE INTERNAL "")
endif()
