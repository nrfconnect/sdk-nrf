#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add GPIO expander image if enabled.
if(SB_CONFIG_DESKTOP_GPIO_EXPANDER)
  if ("${SB_CONFIG_DESKTOP_GPIO_EXPANDER_BOARD}" STREQUAL "")
    message(FATAL_ERROR "GPIO expander board name must be configured")
  endif()

  message(STATUS "Building GPIO expander image for ${SB_CONFIG_DESKTOP_GPIO_EXPANDER_BOARD}")

  # PPR snippet is needed to run PPR on nRF54h20.
  if ((${NORMALIZED_BOARD_TARGET} STREQUAL "nrf54h20dk_nrf54h20_cpuapp") AND
      ("${SB_CONFIG_DESKTOP_GPIO_EXPANDER_BOARD}" STREQUAL "nrf54h20dk/nrf54h20/cpuppr"))
    if(NOT nordic-ppr IN_LIST nrf_desktop_SNIPPET)
      # TODO: Append to snippet instead of replacing
      set(nrf_desktop_SNIPPET nordic-ppr CACHE STRING "" FORCE)
    endif()
  endif()

  ExternalZephyrProject_Add(
    APPLICATION gpio_expander
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../gpio_expander
    BOARD ${SB_CONFIG_DESKTOP_GPIO_EXPANDER_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )

  # Add a dependency so that gpio_expander will be built and flashed first.
  # Required e.g. for PPR use-case.
  sysbuild_add_dependencies(CONFIGURE nrf_desktop gpio_expander)
  sysbuild_add_dependencies(FLASH nrf_desktop gpio_expander)
endif()
