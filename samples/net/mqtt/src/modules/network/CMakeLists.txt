#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# LTE boards
target_sources_ifdef(CONFIG_NRF_MODEM_LIB
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/network_lte.c)

# Wifi boards
target_sources_ifdef(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/network_wifi.c)

# Emulator boards
target_sources_ifdef(CONFIG_BOARD_NATIVE_POSIX
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/network_emulation.c)
