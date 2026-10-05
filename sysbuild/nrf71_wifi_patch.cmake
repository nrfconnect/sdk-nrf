#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Forward Wi-Fi ROM patch Kconfig from sysbuild to the primary application image.
# TF-M receives the resolved values through trusted-firmware-m/CMakeLists.txt.
function(forward_nrf71_wifi_patch_config image)
  foreach(config WIFI_NRF71_PATCH
                 WIFI_NRF71_PATCH_AUTO
                 WIFI_NRF71_PATCH_VERSION_0_1_0
                 WIFI_NRF71_PATCH_OVERRIDE_ORIGINS)
    if(DEFINED SB_CONFIG_${config})
      set_config_bool(${image} CONFIG_${config} ${SB_CONFIG_${config}})
    endif()
  endforeach()
endfunction()
