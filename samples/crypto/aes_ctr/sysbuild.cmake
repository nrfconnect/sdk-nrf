#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# For nRF54h20 we need to always include the crypto_service snippet
if (${NORMALIZED_BOARD_TARGET} STREQUAL "nrf54h20dk_nrf54h20_cpuapp")
if(NOT crypto_service IN_LIST SNIPPET)
  set(SNIPPET crypto_service ${SNIPPET} CACHE STRING "" FORCE)
endif()
endif()