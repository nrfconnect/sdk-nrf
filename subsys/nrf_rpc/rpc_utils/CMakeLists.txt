#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory(common)

if(DEFINED CONFIG_NRF_RPC_DEV_INFO_CLIENT OR DEFINED NRF_RPC_CRASH_GEN_CLIENT)
  add_subdirectory(client)
endif()

if(DEFINED CONFIG_NRF_RPC_DEV_INFO_SERVER OR DEFINED NRF_RPC_CRASH_GEN_SERVER)
  add_subdirectory(server)
endif()
