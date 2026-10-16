#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
  ot_rpc_callback.c
  ot_rpc_common.c
  ot_rpc_group.c
)

zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_RPC_CLIENT ot_rpc_lock_empty.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD ot_rpc_lock_zephyr.c)
