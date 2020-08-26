#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_interface_library_named(thingy52_bt_mesh_models)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_THINGY52_MOD thingy52_mod.c)
zephyr_library_sources_ifdef(CONFIG_BT_MESH_THINGY52_CLI thingy52_cli.c)

zephyr_library_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${NRF_DIR}/subsys/bluetooth/mesh)
