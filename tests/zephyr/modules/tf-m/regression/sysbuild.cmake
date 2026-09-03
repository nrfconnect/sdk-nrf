#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_TEST_PROVISIONING_IMAGE)
  # The provisioning image runs before TF-M is installed, so it has to be built
  # for the secure board target, for example:
  #   nrf54l15dk/nrf54l15/cpuapp/ns -> nrf54l15dk/nrf54l15/cpuapp
  string(REGEX REPLACE "/ns$" "" provisioning_image_qualifiers "${BOARD_QUALIFIERS}")
  if(DEFINED BOARD_REVISION)
    set(provisioning_image_board "${BOARD}@${BOARD_REVISION}/${provisioning_image_qualifiers}")
  else()
    set(provisioning_image_board "${BOARD}/${provisioning_image_qualifiers}")
  endif()

  ExternalZephyrProject_Add(
    APPLICATION provisioning_image
    SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/samples/tfm/provisioning_image
    BOARD ${provisioning_image_board}
    BUILD_ONLY true
  )
endif()
