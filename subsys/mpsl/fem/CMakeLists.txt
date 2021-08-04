#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SOC_NRF5340_CPUAPP mpsl_fem_host.c)
zephyr_library_sources_ifndef(CONFIG_SOC_NRF5340_CPUAPP mpsl_fem.c)
