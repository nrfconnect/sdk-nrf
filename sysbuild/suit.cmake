#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_SUIT_BUILD_FLASH_COMPANION)
  # Calculate the network board target
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  list(GET split_board_qualifiers 2 target_cpucluster)

  set(board_target "${BOARD}/${target_soc}/${SB_CONFIG_FLASH_COMPANION_TARGET_CPUCLUSTER}")

  ExternalZephyrProject_Add(
    APPLICATION flash_companion
    SOURCE_DIR "${ZEPHYR_NRF_MODULE_DIR}/samples/suit/flash_companion"
    BOARD ${board_target}
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()

if(SB_CONFIG_SUIT_BUILD_RECOVERY AND NOT SB_CONFIG_SUIT_RECOVERY_APPLICATION_NONE)
  # Calculate the network board target
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  list(GET split_board_qualifiers 2 target_cpucluster)

  set(board_target "${BOARD}/${target_soc}/${target_cpucluster}")

  ExternalZephyrProject_Add(
    APPLICATION recovery
    SOURCE_DIR ${SB_CONFIG_SUIT_RECOVERY_APPLICATION_PATH}
    BOARD ${board_target}
    BOARD_REVISION ${BOARD_REVISION}
  )

  if(SB_CONFIG_SUIT_RECOVERY_APPLICATION_IMAGE_MANIFEST_APP_RECOVERY)
    set_config_bool(recovery CONFIG_SUIT_LOCAL_ENVELOPE_GENERATE n)
  endif()
endif()

if(SB_CONFIG_SUIT_BUILD_AB_UPDATE)
  cmake_path(APPEND APP_DIR "sysbuild" OUTPUT_VARIABLE slot_overlay_dir)

  get_property(images GLOBAL PROPERTY sysbuild_images)
  foreach(image ${images})
    cmake_path(APPEND slot_overlay_dir "${image}_slot_b.overlay" OUTPUT_VARIABLE overlay)
    if(NOT EXISTS ${overlay})
      continue()
    else()
      message(STATUS "Generate slot B for image: ${image}")
    endif()

    set(variant_image "${image}_variant_b")
    ExternalNcsVariantProject_Add(
      APPLICATION ${image}
      VARIANT ${variant_image}
      SPLIT_KCONFIG true
    )
    # Choose slot_b_partition as the active code partition
    add_overlay_dts(${variant_image} "${overlay}")
    # Disable BICR and UICR generation
    set_config_bool(${variant_image} CONFIG_SOC_NRF54H20_GENERATE_BICR n)
    set_config_bool(${variant_image} CONFIG_NRF_REGTOOL_GENERATE_UICR n)
  endforeach()
endif()
