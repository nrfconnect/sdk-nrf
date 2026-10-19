#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_DECT_NR_RPC_CLIENT client)
add_subdirectory_ifdef(CONFIG_DECT_NR_RPC_SERVER server)
