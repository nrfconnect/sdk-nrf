#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
target_sources_ifdef(CONFIG_FEM app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fem.c)

if (CONFIG_MPSL_FEM_NRF21540_GPIO OR CONFIG_MPSL_FEM_NRF21540_GPIO_SPI)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nrf21540.c)
endif()

target_sources_ifdef(CONFIG_MPSL_FEM_SIMPLE_GPIO app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/generic_fem.c)
