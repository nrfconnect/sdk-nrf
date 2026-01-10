#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lte_net_if.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lte_ip_addr_helper.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lte_cell_prfl_helper.c)

# Add NTN source file when NTN is enabled
target_sources_ifdef(CONFIG_NRF_MODEM_LIB_NET_IF_NTN app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lte_net_if_ntn.c)
