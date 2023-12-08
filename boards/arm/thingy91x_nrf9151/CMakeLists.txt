# Kconfig - nRF91 Thingy:91 X board configuration
#
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library()
if(CONFIG_BOARD_THINGY91X_NRF9151_NS)
	# Use static partition layout to ensure the partition layout remains
	# unchanged after DFU. This needs to be made globally available
	# because it is used in other CMake files.
	set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_pm_static.yml CACHE INTERNAL "")
endif()
