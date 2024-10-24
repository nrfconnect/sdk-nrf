# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Update eGPIO images KConfig based on SDP configuration.
#
# Usage:
#   egpio_update_kconfig()
#
function(egpio_update_kconfig)
  if(SB_CONFIG_EGPIO_BACKEND_MBOX)
    foreach(image ${PRE_CMAKE_IMAGES})
      set_config_bool(${image} CONFIG_GPIO_NRFE_EGPIO_BACKEND_MBOX y)
    endforeach()
    if(DEFINED SB_CONFIG_SOC_NRF54L15)
      sysbuild_cache_set(VAR flpr_egpio_EXTRA_DTC_OVERLAY_FILE APPEND REMOVE_DUPLICATES "./boards/nrf54l15dk_nrf54l15_cpuflpr_mbox.overlay")
    endif()
    sysbuild_cache_set(VAR ${DEFAULT_IMAGE}_SNIPPET APPEND REMOVE_DUPLICATES "sdp-gpio-mbox")
    message(STATUS "eGPIO: Using MBOX backend")
  elseif(SB_CONFIG_EGPIO_BACKEND_ICMSG)
    foreach(image ${PRE_CMAKE_IMAGES})
      set_config_bool(${image} CONFIG_GPIO_NRFE_EGPIO_BACKEND_ICMSG y)
    endforeach()
    set_config_bool(flpr_egpio CONFIG_IPC_SERVICE y)
    set_config_bool(flpr_egpio CONFIG_IPC_SERVICE_BACKEND_ICMSG y)
    if(DEFINED SB_CONFIG_SOC_NRF54L15)
      sysbuild_cache_set(VAR flpr_egpio_EXTRA_DTC_OVERLAY_FILE APPEND REMOVE_DUPLICATES "./boards/nrf54l15dk_nrf54l15_cpuflpr_icmsg.overlay")
    endif()
    sysbuild_cache_set(VAR ${DEFAULT_IMAGE}_SNIPPET APPEND REMOVE_DUPLICATES "sdp-gpio-icmsg")
    message(STATUS "eGPIO: Using ICMSG backend")
  elseif(SB_CONFIG_EGPIO_BACKEND_ICBMSG)
    foreach(image ${PRE_CMAKE_IMAGES})
      set_config_bool(${image} CONFIG_GPIO_NRFE_EGPIO_BACKEND_ICBMSG y)
      set_property(TARGET ${image} APPEND_STRING PROPERTY CONFIG "CONFIG_IPC_SERVICE_BACKEND_ICBMSG_NUM_EP=1\n")
    endforeach()
    set_config_bool(flpr_egpio CONFIG_IPC_SERVICE y)
    set_config_bool(flpr_egpio CONFIG_IPC_SERVICE_BACKEND_ICBMSG y)
    if(DEFINED SB_CONFIG_SOC_NRF54L15)
      sysbuild_cache_set(VAR flpr_egpio_EXTRA_DTC_OVERLAY_FILE APPEND REMOVE_DUPLICATES "./boards/nrf54l15dk_nrf54l15_cpuflpr_icbmsg.overlay")
    endif()
    sysbuild_cache_set(VAR ${DEFAULT_IMAGE}_SNIPPET APPEND REMOVE_DUPLICATES "sdp-gpio-icbmsg")
    message(STATUS "eGPIO: Using ICBMSG backend")
  endif()
endfunction()

# If eGPIO FLPR application is enabled, update Kconfigs
if(SB_CONFIG_EGPIO_FLPR_APPLICATION)
  egpio_update_kconfig()
endif()
