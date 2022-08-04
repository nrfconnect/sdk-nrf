#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_MPSL_FEM_NCS_SUPPORTED_FEM_USED)
    zephyr_library()

    zephyr_library_include_directories(common/include)
    zephyr_library_sources(common/mpsl_fem_utils.c)

    zephyr_library_sources_ifdef(CONFIG_MPSL_FEM_NRF21540_GPIO nrf21540_gpio/mpsl_fem_nrf21540_gpio.c)

    add_subdirectory_ifdef(CONFIG_MPSL_FEM_NRF21540_GPIO_SPI nrf21540_gpio_spi)

    zephyr_library_sources_ifdef(CONFIG_MPSL_FEM_SIMPLE_GPIO simple_gpio/mpsl_fem_simple_gpio.c)
endif()
