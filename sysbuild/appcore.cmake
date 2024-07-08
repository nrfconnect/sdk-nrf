#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)

# Include application core image if enabled
if(SB_CONFIG_SUPPORT_APPCORE AND NOT SB_CONFIG_APPCORE_NONE AND DEFINED SB_CONFIG_APPCORE_IMAGE_NAME)
  # Calculate the application board target
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  list(GET split_board_qualifiers 2 target_cpucluster)

  set(board_target_appcore "${BOARD}/${target_soc}/${SB_CONFIG_APPCORE_REMOTE_BOARD_TARGET_CPUCLUSTER}")

  set(target_soc)
  set(target_cpucluster)

  ExternalZephyrProject_Add(
    APPLICATION ${SB_CONFIG_APPCORE_IMAGE_NAME}
    SOURCE_DIR ${SB_CONFIG_APPCORE_IMAGE_PATH}
    BOARD ${board_target_appcore}
    BOARD_REVISION ${BOARD_REVISION}
  )

  if(NOT "${SB_CONFIG_APPCORE_IMAGE_DOMAIN}" IN_LIST PM_DOMAINS)
    list(APPEND PM_DOMAINS ${SB_CONFIG_APPCORE_IMAGE_DOMAIN})
  endif()

  set_property(GLOBAL APPEND PROPERTY
               PM_${SB_CONFIG_APPCORE_IMAGE_DOMAIN}_IMAGES
               "${SB_CONFIG_APPCORE_IMAGE_NAME}"
  )

  set_property(GLOBAL PROPERTY DOMAIN_APP_${SB_CONFIG_APPCORE_IMAGE_DOMAIN}
               "${SB_CONFIG_APPCORE_IMAGE_NAME}"
  )

  set(${SB_CONFIG_APPCORE_IMAGE_DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION
      ${SB_CONFIG_APPCORE_IMAGE_NAME} CACHE INTERNAL ""
  )

  set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
endif()
