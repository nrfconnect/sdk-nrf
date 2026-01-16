#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(
  dect_mdm.c
  dect_mdm_ctrl.c
  dect_mdm_ctrl_api.c
  dect_mdm_ctrl_mdm.c
  dect_mdm_rx.c
  dect_mdm_settings.c
  dect_mdm_utils.c
)
zephyr_library_sources_ifdef(CONFIG_NET_L2_DECT_BR dect_mdm_sink.c)

# For IPv6 address handling
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
