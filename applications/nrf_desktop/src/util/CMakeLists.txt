#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hwid.c)

target_sources_ifdef(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config_channel_transport.c)

target_sources_ifdef(CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dfu_lock.c)

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
