#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

# This is needed to get the library paths for cbor attrs etc.
# used by MCUMgr but also reused in this module
zephyr_library_link_libraries(MCUMGR)

zephyr_library_sources(
  src/fmfu_mgmt.c
  src/fmfu_mgmt_stat.c
  )
