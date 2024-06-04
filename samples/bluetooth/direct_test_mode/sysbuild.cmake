#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)

# Include app core image if enabled
if(SB_CONFIG_SUPPORT_APPCORE AND DEFINED SB_CONFIG_APPCORE_IMAGE_NAME)
  ExternalZephyrProject_Add(
    APPLICATION ${SB_CONFIG_APPCORE_IMAGE_NAME}
    SOURCE_DIR ${SB_CONFIG_APPCORE_IMAGE_PATH}
    BOARD ${SB_CONFIG_APPCORE_REMOTE_BOARD_NAME}
  )

  if(NOT "CPUAPP" IN_LIST PM_DOMAINS)
    list(APPEND PM_DOMAINS CPUAPP)
  endif()

  set_property(GLOBAL APPEND PROPERTY PM_CPUAPP_IMAGES "${SB_CONFIG_APPCORE_IMAGE_NAME}")
  set_property(GLOBAL PROPERTY DOMAIN_APP_CPUAPP "${SB_CONFIG_APPCORE_IMAGE_NAME}")
  set(CPUAPP_PM_DOMAIN_DYNAMIC_PARTITION ${SB_CONFIG_APPCORE_IMAGE_NAME} CACHE INTERNAL "")

  string(REPLACE " " ";" shield_list "${SHIELD}")
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
  set_config_bool(direct_test_mode CONFIG_DTM_TRANSPORT_HCI y)
else()
  set_config_bool(direct_test_mode CONFIG_DTM_TRANSPORT_HCI n)
endif()

set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
