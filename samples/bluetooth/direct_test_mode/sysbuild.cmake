#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Include app core image if enabled
if(SB_CONFIG_SOC_NRF5340_CPUNET AND DEFINED SB_CONFIG_APPCORE_IMAGE_NAME)
  if("nrf21540ek" IN_LIST shield_list)
    add_overlay_dts(
      ${SB_CONFIG_APPCORE_IMAGE_NAME}
      ${CMAKE_CURRENT_LIST_DIR}/sysbuild/remote_shell/nrf21540ek.overlay)
  endif()
endif()

if(SB_CONFIG_DTM_NO_DFE)
  add_overlay_dts(
    direct_test_mode
    ${CMAKE_CURRENT_LIST_DIR}/no-dfe.overlay)
endif()

if(SB_CONFIG_DTM_TRANSPORT_HCI)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_DTM_TRANSPORT_HCI y)
else()
  set_config_bool(${DEFAULT_IMAGE} CONFIG_DTM_TRANSPORT_HCI n)
endif()
