# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Update FLPR image KConfig based on main app configuration.
#
# Usage:
#   flpr_egpio_update_kconfig()
#
function(flpr_egpio_update_kconfig)

  sysbuild_get(CONFIG_MBOX_BACKEND IMAGE ${DEFAULT_IMAGE} VAR CONFIG_GPIO_NRFE_MBOX_BACKEND KCONFIG)
  sysbuild_get(CONFIG_ICMSG_BACKEND IMAGE ${DEFAULT_IMAGE} VAR CONFIG_GPIO_NRFE_ICMSG_BACKEND KCONFIG)

  if(CONFIG_MBOX_BACKEND)
    sysbuild_cache_set(VAR egpio_CONFIG_GPIO_NRFE_MBOX_BACKEND APPEND REMOVE_DUPLICATES "y")
    message(STATUS "eGPIO: Using MBOX backend")
  elseif(CONFIG_ICMSG_BACKEND)
    sysbuild_cache_set(VAR egpio_CONFIG_GPIO_NRFE_ICMSG_BACKEND APPEND REMOVE_DUPLICATES "y")
    sysbuild_cache_set(VAR egpio_EXTRA_DTC_OVERLAY_FILE APPEND REMOVE_DUPLICATES "./boards/nrf54l15pdk_nrf54l15_cpuflpr_icmsg.overlay")
    sysbuild_cache_set(VAR egpio_CONFIG_IPC_SERVICE APPEND REMOVE_DUPLICATES "y")
    sysbuild_cache_set(VAR egpio_CONFIG_IPC_SERVICE_BACKEND_ICMSG APPEND REMOVE_DUPLICATES "y")
    message(STATUS "eGPIO: Using ICMSG backend")
  endif()

  sysbuild_cache(CREATE APPLICATION egpio CMAKE_RERUN)

endfunction()

# If eGPIO FLPR application is enabled, update Kconfigs
if(SB_CONFIG_EGPIO_FLPR_APPLICATION)
  flpr_egpio_update_kconfig()
endif()
