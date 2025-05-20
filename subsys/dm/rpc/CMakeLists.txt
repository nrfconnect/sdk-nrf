#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_DM_MODULE_RPC_CLIENT client)
add_subdirectory_ifdef(CONFIG_DM_MODULE_RPC_HOST host)
