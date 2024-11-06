#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources_ifdef(
  CONFIG_BT_RAS_RRSP
  ras_rrsp.c
  ras_rd_buffer.c)
