#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_MCUMGR_GRP_OS_BOOTLOADER_INFO_B0_ACTIVE_SLOT)
  zephyr_library_amend()
  zephyr_library_sources(src/os_mgmt_b0_active_slot.c)
endif()
