#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_NFC_RPC_CLIENT client)
add_subdirectory_ifdef(CONFIG_NFC_RPC_SERVER server)
