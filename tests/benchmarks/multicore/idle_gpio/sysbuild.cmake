#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_SOC_NRF54H20 AND NOT SB_CONFIG_NETCORE_NONE)
  set_config_bool(idle_gpio CONFIG_SOC_NRF54H20_CPURAD_ENABLE y)
endif()

if(NOT SB_CONFIG_PPRCORE_NONE)
  set(idle_gpio_SNIPPET nordic-ppr CACHE STRING "" FORCE)
endif()
