#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

file(GLOB sample_images LIST_DIRECTORIES true ${APP_DIR}/images/*)

foreach(image ${sample_images})
#  set(image "unsigned_uuids")
  get_filename_component(image_name ${image} NAME)

  ExternalZephyrProject_Add(
    APPLICATION ${image_name}
    SOURCE_DIR ${APP_DIR}/images/${image_name}
    BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_BOARD_QUALIFIERS}
    BOARD_REVISION ${BOARD_REVISION}
  )
  UpdateableImage_Add(
    APPLICATION ${image_name}
  )

  if(SB_CONFIG_MCUBOOT_SYSBUILD_SIGN)
    set(${image_name}_SIGNING_SCRIPT "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing_redirect.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)
  else()
    set(${image_name}_SIGNING_SCRIPT "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)
  endif()
endforeach()
