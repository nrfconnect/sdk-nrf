#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lte_net_if.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lte_ip_addr_helper.c)
