#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_include_directories(${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_SILVAIR_ENOCEAN_SRV silvair_enocean_srv.c)
zephyr_library_sources_ifdef(CONFIG_BT_MESH_DM_SRV dm_srv.c)
zephyr_library_sources_ifdef(CONFIG_BT_MESH_DM_CLI dm_cli.c)
