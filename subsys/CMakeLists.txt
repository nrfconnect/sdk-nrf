#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_BT OR CONFIG_BT_LL_NRFXLIB)
  add_subdirectory(bluetooth)
endif()

add_subdirectory_ifdef(CONFIG_IS_SECURE_BOOTLOADER bootloader)
add_subdirectory_ifdef(CONFIG_SECURE_BOOT bootloader/image)

add_subdirectory(net)
add_subdirectory_ifdef(CONFIG_NRF_ESB		enhanced_shockburst)
add_subdirectory_ifdef(CONFIG_EVENT_MANAGER	event_manager)
add_subdirectory_ifdef(CONFIG_PROFILER		profiler)
add_subdirectory_ifdef(CONFIG_NRFXLIB_NFC	nfc)
add_subdirectory_ifdef(CONFIG_IS_SPM spm)
add_subdirectory_ifdef(CONFIG_FW_METADATA	fw_metadata)

add_subdirectory(partition_manager)
