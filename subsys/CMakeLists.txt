#
# Copyright (c) 2018 - 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_BT bluetooth)

add_subdirectory_ifdef(CONFIG_BOOTLOADER_MCUBOOT bootloader/bl_override)
add_subdirectory_ifdef(CONFIG_IS_SECURE_BOOTLOADER bootloader)
if(NOT (SYSBUILD OR NCS_SYSBUILD_PARTITION_MANAGER))
  add_subdirectory_ifdef(CONFIG_SECURE_BOOT bootloader/image)
endif()
add_subdirectory_ifdef(CONFIG_SECURE_BOOT_CRYPTO bootloader/bl_crypto)
add_subdirectory_ifdef(CONFIG_SECURE_BOOT_VALIDATION bootloader/bl_validation)
add_subdirectory_ifdef(CONFIG_SECURE_BOOT_STORAGE bootloader/bl_storage)

if(NOT SYSBUILD)
  if(CONFIG_BOOTLOADER_PROVISION_HEX)
    include(bootloader/cmake/provision_hex.cmake)
  endif()
endif()

add_subdirectory_ifdef(CONFIG_NRF_SECURITY nrf_security)
add_subdirectory_ifdef(CONFIG_TRUSTED_STORAGE trusted_storage)

add_subdirectory(net)
add_subdirectory_ifdef(CONFIG_ESB		esb)
add_subdirectory_ifdef(CONFIG_APP_EVENT_MANAGER	app_event_manager)
add_subdirectory_ifdef(CONFIG_APP_EVENT_MANAGER	app_event_manager_profiler_tracer)
add_subdirectory_ifdef(CONFIG_EVENT_MANAGER_PROXY	event_manager_proxy)
add_subdirectory_ifdef(CONFIG_CAF		caf)
add_subdirectory_ifdef(CONFIG_NRF_PROFILER		nrf_profiler)
add_subdirectory(dfu)
if (CONFIG_PCD OR CONFIG_PCD_APP OR CONFIG_PCD_NET)
add_subdirectory(pcd)
endif()

add_subdirectory(mpsl)

add_subdirectory_ifdef(CONFIG_ZIGBEE zigbee)
add_subdirectory_ifdef(CONFIG_MGMT_FMFU mgmt/fmfu)
add_subdirectory_ifdef(CONFIG_GAZELL gazell)

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
if(CONFIG_FW_INFO OR CONFIG_FW_INFO_API)
  add_subdirectory(fw_info)
endif()
add_subdirectory(shell)

add_subdirectory(debug)
add_subdirectory_ifdef(CONFIG_PARTITION_MANAGER_ENABLED partition_manager)

add_subdirectory_ifdef(CONFIG_NRF_RPC nrf_rpc)
add_subdirectory_ifdef(CONFIG_NRF_802154_RADIO_DRIVER ieee802154)
add_subdirectory_ifdef(CONFIG_NRF_DM dm)
add_subdirectory_ifdef(CONFIG_EMDS emds)
add_subdirectory_ifdef(CONFIG_NET_CORE_MONITOR net_core_monitor)
add_subdirectory_ifdef(CONFIG_AUDIO_MODULE audio_module)
add_subdirectory_ifdef(CONFIG_UART_ASYNC_ADAPTER uart_async_adapter)
add_subdirectory_ifdef(CONFIG_SDFW_SERVICES_ENABLED sdfw_services)
add_subdirectory(suit)
add_subdirectory_ifdef(CONFIG_MGMT_SUITFU mgmt/suitfu)
add_subdirectory_ifdef(CONFIG_DULT dult)
