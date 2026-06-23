#
# Copyright (c) 2024-2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_APP_DFU)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_APP_DFU y)
else()
  set_config_bool(${DEFAULT_IMAGE} CONFIG_APP_DFU n)
endif()
