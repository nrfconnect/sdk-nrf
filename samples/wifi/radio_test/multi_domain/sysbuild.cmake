#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_SUPPORT_NETCORE_PERIPHERAL_RADIO_TEST)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_SOC_NRF53_CPUNET_ENABLE y)
endif()
