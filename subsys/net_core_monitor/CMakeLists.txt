#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_SOC_NRF5340_CPUAPP)
	zephyr_library_sources(app_core.c)
endif()

if(CONFIG_SOC_NRF5340_CPUNET)
	zephyr_library_sources(net_core.c)
endif()
