#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources_ifdef(
  CONFIG_BT_CGMS
  cgms.c
  cgms_socp.c
  cgms_racp.c)
