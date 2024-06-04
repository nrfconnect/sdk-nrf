#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_sources_ifdef(CONFIG_DESKTOP_HWID
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hwid.c)

if(CONFIG_DESKTOP_HWINFO_BLE_ADDRESS_FICR)
         message(WARNING
           "CONFIG_DESKTOP_HWINFO_BLE_ADDRESS_FICR is enabled. "
           "Devices with this HWINFO driver are using BLE address from the FICR register "
           "expanded with a value defined in the CONFIG_DESKTOP_HWINFO_BLE_ADDRESS_FICR_POSTFIX "
           "Kconfig option. If this register is not populated, "
           "you can use the postfix to ensure unique HW IDs for your devices. "
           "Otherwise, all devices will have the same HW ID.")
           target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hwinfo_ble_addr_ficr.c)
endif()

target_sources_ifdef(CONFIG_DESKTOP_ADV_PROV_UUID16_ALL
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bt_le_adv_prov_uuid16.c)

target_sources_ifdef(CONFIG_DESKTOP_DFU_LOCK
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dfu_lock.c)

target_sources_ifdef(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config_channel_transport.c)

if(CONFIG_DESKTOP_BLE_QOS_ENABLE)
  if(CONFIG_FPU)
    if(CONFIG_FP_HARDABI)
      set(float_dir hard-float)
    elseif(CONFIG_FP_SOFTABI)
      set(float_dir softfp-float)
    else()
      assert(0 "Unreachable code")
    endif()
  else()
    set(float_dir soft-float)
  endif()
  zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/chmap_filter/lib/${GCC_M_CPU}/${float_dir}/libchmapfilt.a)
  target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/chmap_filter/include)
endif()
