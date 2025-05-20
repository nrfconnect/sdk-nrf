#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_PSA_CORE_LITE)
  add_subdirectory(lite)
else()
  add_subdirectory(nrf_oberon)
endif()
