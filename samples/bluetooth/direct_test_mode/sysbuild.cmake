#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if("${BOARD}" STREQUAL "nrf5340dk_nrf5340_cpunet")
  # Add remote project
  if(SB_CONFIG_DTM_APP_REMOTE_SHELL)
    set(remote_image ${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/remote_shell)
    set(remote_image_name remote_shell)
  else()
    set(remote_image ${APP_DIR}/remote_hci)
    set(remote_image_name remote_hci)
  endif()

  set_target_properties(direct_test_mode PROPERTIES MAIN_APP False)
  set_target_properties(direct_test_mode PROPERTIES IMAGE_CONF_SCRIPT "")

  ExternalZephyrProject_Add(
      APPLICATION ${remote_image_name}
      SOURCE_DIR ${remote_image}
      BOARD nrf5340dk_nrf5340_cpuapp
    )

  set_target_properties(${remote_image_name} PROPERTIES MAIN_APP False)
  set_target_properties(${remote_image_name} PROPERTIES IMAGE_CONF_SCRIPT "${CMAKE_SOURCE_DIR}/image_configurations/MAIN_image_default.cmake")

  set(DEFAULT_IMAGE ${remote_image_name} PARENT_SCOPE)

  set_property(GLOBAL APPEND PROPERTY PM_DOMAINS CPUNET)
  set_property(GLOBAL APPEND PROPERTY PM_CPUNET_IMAGES direct_test_mode)
  set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET direct_test_mode)
  set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION direct_test_mode CACHE INTERNAL "")

  # Add a dependency so that the remote sample will be built and flashed first
  add_dependencies(direct_test_mode ${remote_image_name})
  # Add dependency so that the remote image is flashed first.
  sysbuild_add_dependencies(FLASH direct_test_mode ${remote_image_name})

  if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/boards/${BOARD}.conf)
    add_overlay_config(
      direct_test_mode
      ${CMAKE_CURRENT_LIST_DIR}/boards/${BOARD}.conf
      )
  endif()

  if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/boards/${BOARD}.overlay)
    add_overlay_dts(
      direct_test_mode
      ${CMAKE_CURRENT_LIST_DIR}/boards/${BOARD}.overlay
      )
  endif()

  if(SB_CONFIG_DTM_USB)
    set_config_bool(direct_test_mode CONFIG_DTM_USB y)

    add_overlay_dts(
      ${remote_image_name}
      ${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell/usb.overlay
      )

    add_overlay_config(
      ${remote_image_name}
      ${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell/prj_usb.conf
      )
  else()
    set_config_bool(direct_test_mode CONFIG_DTM_USB n)
  endif()

  if(NOT "${direct_test_mode_EXTRA_CONF_FILE}" STREQUAL "overlay-hci-nrf53.conf")
    add_overlay_dts(
      direct_test_mode
      ${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell_nrf53.overlay
      )
  endif()
endif()
