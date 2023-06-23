#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(SB_CONFIG_SECURE_BOOT)
  if(SB_CONFIG_SECURE_BOOT_NETCORE)
    set(secure_boot_source_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/netboot)
  else()
    set(secure_boot_source_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/bootloader)
  endif()
  ExternalZephyrProject_Add(
    APPLICATION ${SB_CONFIG_SECURE_BOOT_IMAGE_NAME}
    # ToDo: which sample is used as  secure boot sample in other cases ?
    SOURCE_DIR ${secure_boot_source_dir}
    BOARD ${SB_CONFIG_SECURE_BOOT_BOARD}
  )
  list(APPEND IMAGES "${SB_CONFIG_SECURE_BOOT_IMAGE_NAME}")
  set_target_properties(${SB_CONFIG_SECURE_BOOT_IMAGE_NAME} PROPERTIES
    IMAGE_CONF_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/image_configurations/b0_image_default.cmake
  )

  # Can we do this in a more clever way ?
  # Should adding of hci_rpmasg be a downstream feature, or could we perhaps
  # overload upstream image adding.
  if(NOT "${SB_CONFIG_SECURE_BOOT_DOMAIN}" IN_LIST PM_DOMAINS)
    list(APPEND PM_DOMAINS ${SB_CONFIG_SECURE_BOOT_DOMAIN})
  endif()
  list(APPEND PM_${SB_CONFIG_SECURE_BOOT_DOMAIN}_IMAGES
      "${SB_CONFIG_SECURE_BOOT_IMAGE_NAME}"
  )
  set(PM_${SB_CONFIG_SECURE_BOOT_DOMAIN}_IMAGES
      ${PM_${SB_CONFIG_SECURE_BOOT_DOMAIN}_IMAGES}
      PARENT_SCOPE
  )

  if(SB_CONFIG_SECURE_BOOT_DOMAIN_APP)
    set(DOMAIN_APP_${SB_CONFIG_SECURE_BOOT_DOMAIN} "${SB_CONFIG_SECURE_BOOT_IMAGE_NAME}" PARENT_SCOPE)
  endif()
endif()

set(PM_DOMAINS ${PM_DOMAINS} PARENT_SCOPE)
set(IMAGES     ${IMAGES}     PARENT_SCOPE)
