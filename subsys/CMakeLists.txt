#
# Copyright (c) 2018 - 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_BT)
  add_subdirectory(bluetooth)
endif()

add_subdirectory_ifdef(CONFIG_IS_SECURE_BOOTLOADER bootloader)
add_subdirectory_ifdef(CONFIG_SECURE_BOOT bootloader/image)
add_subdirectory_ifdef(CONFIG_SECURE_BOOT_CRYPTO bootloader/bl_crypto)
add_subdirectory_ifdef(CONFIG_SECURE_BOOT_VALIDATION bootloader/bl_validation)
add_subdirectory_ifdef(CONFIG_SECURE_BOOT_STORAGE bootloader/bl_storage)

add_subdirectory(net)
add_subdirectory_ifdef(CONFIG_ESB		esb)
add_subdirectory_ifdef(CONFIG_EVENT_MANAGER	event_manager)
add_subdirectory_ifdef(CONFIG_PROFILER		profiler)
add_subdirectory(dfu)
add_subdirectory_ifdef(CONFIG_PCD 		pcd)
add_subdirectory_ifdef(CONFIG_IS_SPM		spm)
add_subdirectory_ifdef(CONFIG_TRUSTED_EXECUTION_NONSECURE nonsecure)
add_subdirectory_ifdef(CONFIG_MPSL mpsl)
add_subdirectory_ifdef(CONFIG_ZIGBEE zigbee)
add_subdirectory_ifdef(CONFIG_MGMT_FMFU mgmt/fmfu)

if (CONFIG_NFC_T2T_NRFXLIB OR
    CONFIG_NFC_T4T_NRFXLIB OR
    CONFIG_NFC_T2T_PARSER  OR
    CONFIG_NFC_NDEF_PARSER OR
    CONFIG_NFC_T4T_ISODEP OR
    CONFIG_NFC_T4T_APDU OR
    CONFIG_NFC_T4T_CC_FILE OR
    CONFIG_NFC_T4T_HL_PROCEDURE OR
    CONFIG_NFC_TNEP_TAG OR
    CONFIG_NFC_TNEP_POLLER)
        add_subdirectory(nfc)
endif()

add_subdirectory_ifdef(CONFIG_FW_INFO	fw_info)
add_subdirectory_ifdef(CONFIG_SHELL_BT_NUS	shell)

add_subdirectory(debug)
add_subdirectory(partition_manager)

add_subdirectory_ifdef(CONFIG_NRF_RPC nrf_rpc)
