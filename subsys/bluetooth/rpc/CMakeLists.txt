#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_interface_library_named(subsys_bluetooth_rpc)

target_include_directories(subsys_bluetooth_rpc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_BT_RPC_CLIENT client)
add_subdirectory_ifdef(CONFIG_BT_RPC_HOST host)
add_subdirectory(soc)
