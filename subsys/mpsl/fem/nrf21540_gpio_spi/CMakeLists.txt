#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources(mpsl_fem_nrf21540_gpio_spi.c)

zephyr_library_sources_ifdef(CONFIG_MPSL_FEM_POWER_MODEL_NRF21540_USE_BUILTIN models/mpsl_fem_nrf21540_power_model_builtin.c)
