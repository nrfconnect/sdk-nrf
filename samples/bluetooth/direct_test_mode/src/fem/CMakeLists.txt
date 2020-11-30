#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)
target_sources_ifdef(CONFIG_NRF21540_FEM app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nrf21540.c)
