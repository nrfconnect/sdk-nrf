#
# Copyright (c) 2024-2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_MCUMGR_GRP_OS_BOOTLOADER_INFO_B0_ACTIVE_SLOT)
  # Amend the os management group library
  set(ZEPHYR_CURRENT_LIBRARY "mcumgr_grp_os_mgmt")
  zephyr_library_sources(src/os_mgmt_b0_active_slot.c)
endif()

if(CONFIG_MCUMGR_GRP_OS_REBOOT_BT)
  # Amend the os management group library
  set(ZEPHYR_CURRENT_LIBRARY "mcumgr_grp_os_mgmt")
  zephyr_library_sources(src/os_mgmt_reboot_bt.c)
endif()
