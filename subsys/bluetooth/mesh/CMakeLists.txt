#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources_ifdef(CONFIG_BT_MESH_NORDIC_MODELS model_utils.c)

zephyr_sources_ifdef(CONFIG_BT_MESH_ONOFF_SRV gen_onoff_srv.c)
zephyr_sources_ifdef(CONFIG_BT_MESH_ONOFF_CLI gen_onoff_cli.c)

zephyr_sources_ifdef(CONFIG_BT_MESH_LVL_SRV gen_lvl_srv.c)
zephyr_sources_ifdef(CONFIG_BT_MESH_LVL_CLI gen_lvl_cli.c)

zephyr_sources_ifdef(CONFIG_BT_MESH_DTT_SRV gen_dtt_srv.c)
zephyr_sources_ifdef(CONFIG_BT_MESH_DTT_CLI gen_dtt_cli.c)
